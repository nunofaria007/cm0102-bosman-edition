#include "exe_image.h"
#include <CommonCrypto/CommonDigest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The cm0102.exe 3.9.68 that the differential test also checks. */
static const char g_exe_sha256[] =
    "0c78e7da6d0bcb4603fa179449bbe6d1b6b5e1c627a22c445d14d489bd9bff86";

static u8 *g_file;
static size_t g_file_size;

/* A section of the PE file: its address range and where its data lies. */
typedef struct {
    u32 address;
    u32 size;       /* bytes in the file */
    u32 offset;
} image_section_t;

static image_section_t g_sections[16];
static u32 g_section_count;

static u32 le16(const u8 *p) { return (u32)p[0] | (u32)p[1] << 8; }
static u32 le32(const u8 *p) { return le16(p) | le16(p + 2) << 16; }

/* Reads the section table. False if the file is not a PE32 file. */
static bool read_sections(const u8 *data, size_t size, image_section_t *out, u32 *count)
{
    u32 pe, sections, optional, base;

    if (size < 0x40 || data[0] != 'M' || data[1] != 'Z') return false;
    pe = le32(data + 0x3c);
    if (pe > size - 0x78 || memcmp(data + pe, "PE\0\0", 4)) return false;
    sections = le16(data + pe + 6);
    optional = le16(data + pe + 20);
    base = le32(data + pe + 24 + 28);
    if (sections > 16 || pe + 24 + optional + sections * 40 > size) return false;
    for (u32 i = 0; i < sections; i++) {
        const u8 *s = data + pe + 24 + optional + i * 40;
        u32 vsize = le32(s + 8), rsize = le32(s + 16);

        out[i].address = base + le32(s + 12);
        out[i].size = rsize < vsize ? rsize : vsize;
        out[i].offset = le32(s + 20);
        if ((unsigned long long)out[i].offset + out[i].size > size) return false;
    }
    *count = sections;
    return true;
}

bool exe_image_load(const char *exe_path, char *error, size_t error_size)
{
    FILE *file = fopen(exe_path, "rb");
    u8 *data = NULL;
    long size = 0;
    unsigned char digest[CC_SHA256_DIGEST_LENGTH];
    char hex[2 * CC_SHA256_DIGEST_LENGTH + 1] = "";
    image_section_t sections[16];
    u32 count = 0;

    if (!file) {
        snprintf(error, error_size,
                 "Cannot open %s. The game needs cm0102.exe of Championship "
                 "Manager 01/02 3.9.68 for its random numbers and names. "
                 "Use --exe <path>.", exe_path);
        return false;
    }
    if (fseek(file, 0, SEEK_END) == 0 && (size = ftell(file)) > 0 &&
        fseek(file, 0, SEEK_SET) == 0 && (data = malloc((size_t)size)) &&
        fread(data, 1, (size_t)size, file) == (size_t)size) {
        CC_SHA256(data, (CC_LONG)size, digest);
        for (int i = 0; i < CC_SHA256_DIGEST_LENGTH; i++) {
            snprintf(hex + 2 * i, 3, "%02x", digest[i]);
        }
    } else {
        size = 0;
    }
    fclose(file);
    if (!size || strcmp(hex, g_exe_sha256) ||
        !read_sections(data, (size_t)size, sections, &count)) {
        snprintf(error, error_size,
                 "%s is not cm0102.exe of Championship Manager 01/02 3.9.68 "
                 "(SHA256 %s). Use --exe <path>.", exe_path,
                 size ? hex : "unknown");
        free(data);
        return false;
    }
    free(g_file);
    g_file = data;
    g_file_size = (size_t)size;
    memcpy(g_sections, sections, sizeof(sections));
    g_section_count = count;
    return true;
}

bool exe_image_loaded(void)
{
    return g_file != NULL;
}

const u8 *exe_image_file(size_t *size)
{
    if (size) *size = g_file_size;
    return g_file;
}

const char *exe_image_string(u32 address)
{
    if (!g_file) {
        fprintf(stderr, "cm0102.exe is not loaded: the names of the key ids "
                        "need it. Use --exe <path>.\n");
        exit(1);
    }
    for (u32 i = 0; i < g_section_count; i++) {
        const image_section_t *s = &g_sections[i];

        if (address >= s->address && address - s->address < s->size) {
            const char *text = (const char *)g_file + s->offset + (address - s->address);

            /* The string must end inside the section data. */
            if (memchr(text, 0, s->size - (address - s->address))) return text;
            return NULL;
        }
    }
    return NULL;
}
