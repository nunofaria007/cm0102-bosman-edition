#include "random.h"
#include <CommonCrypto/CommonDigest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* The cm0102.exe 3.9.68 that the differential test also checks. */
static const char g_exe_sha256[] =
    "0c78e7da6d0bcb4603fa179449bbe6d1b6b5e1c627a22c445d14d489bd9bff86";

/* The table at 0xaa2454 lies at this offset in the exe file (.data). */
#define TABLE_FILE_OFFSET 0x6a2454

/* 0x970a70, a double: 1 / 0xffff. */
static const double g_scale = 1.5259021896696422e-05;

u32 g_holdrand = 1;
s32 g_random_index;
s32 g_random_end = -1;
s32 g_random_offset;
s32 g_random_table[RANDOM_TABLE_SIZE];
char g_random_seed_text[64];

void cm_srand(u32 seed)
{
    g_holdrand = seed;
}

s32 cm_rand(void)
{
    g_holdrand = g_holdrand * 214013u + 2531011u;
    return (s32)(g_holdrand >> 16) & 0x7fff;
}

s32 random_number(s32 n)
{
    if (n == 0) return 0;
    g_random_index++;
    if (g_random_index > g_random_end) {
        g_random_index = 0;
        g_random_offset = cm_rand() & 0xffff;
    }
    s32 value = g_random_offset + g_random_table[g_random_index];
    if (n <= 0xffff && n >= -0xffff) {
        s32 r = value % n;
        return n < 1 ? -r : r;
    }
    /* The original multiplies at the double precision that its CRT sets. */
    s32 low = (s32)((double)(value % 0xffff) * ((double)n * g_scale));
    s32 high = random_number((s32)((u32)n * 10u) / 0xffff);
    s32 sum = (s32)((u32)low + (u32)((high + 5) / 10));
    s32 last = (s32)((u32)n - 1u);
    return sum > last ? last : sum;
}

void random_seed(s32 seed)
{
    if (seed == 0) seed = (s32)time(NULL);
    cm_srand((u32)seed);
    snprintf(g_random_seed_text, sizeof(g_random_seed_text),
             "Random Number seed: %d", seed);
    g_random_index = cm_rand() % RANDOM_TABLE_SIZE;
    g_random_end = RANDOM_TABLE_SIZE - 1;
    g_random_offset = cm_rand() % 0xffff;
    /* The original then puts a pointer outside the table back to its start. */
}

bool random_load_table(const char *exe_path, char *error, size_t error_size)
{
    FILE *file = fopen(exe_path, "rb");
    u8 *data = NULL;
    long size;
    unsigned char digest[CC_SHA256_DIGEST_LENGTH];
    char hex[2 * CC_SHA256_DIGEST_LENGTH + 1];

    if (!file) {
        snprintf(error, error_size,
                 "Cannot open %s. The game needs cm0102.exe of Championship "
                 "Manager 01/02 3.9.68 for its random numbers. "
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
    if (!size || strcmp(hex, g_exe_sha256)) {
        snprintf(error, error_size,
                 "%s is not cm0102.exe of Championship Manager 01/02 3.9.68 "
                 "(SHA256 %s). Use --exe <path>.", exe_path,
                 size ? hex : "unknown");
        free(data);
        return false;
    }
    memcpy(g_random_table, data + TABLE_FILE_OFFSET, sizeof(g_random_table));
    free(data);
    return true;
}
