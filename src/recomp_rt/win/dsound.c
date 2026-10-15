/* DirectSound on SDL audio (inventory 2.2, columns-and-colours inventory section 3).
 *
 * The game's sound object (0x84ca80..0x84cf40) has ten slots. Each play loads one WAV file into a new
 * static buffer (0x84cd10): CreateSoundBuffer with the format of the file, Lock, one fread, Unlock.
 * Then SetPan, SetVolume and Play (0x84cbc0). The game asks GetStatus (mask 5) to know when a sound
 * ends (0x84cc80, 0x84cf00): the commentary waits for it (0x6d8c70). Stop, SetCurrentPosition(0) and
 * Release free a slot (0x84cb70).
 *
 * Here each buffer has a voice of a small mixer. At Unlock the voice converts the PCM once to the
 * format of the device (44100 Hz, stereo, signed 16 bit). The SDL audio callback mixes the voices
 * that play, and moves their positions. So GetStatus tells the truth: a sound plays for as long as
 * its samples take. The SDL dummy audio driver also takes the samples in real time.
 *
 * The settings file option "sound = off" opens no device. Then every buffer is silent and GetStatus
 * says "not playing", as in release 0.1.0. The game's own Sound On/Off setting needs nothing here:
 * with Sound Off the game makes no sound object (0x5d7c60).
 *
 * As in DirectSound without DSBCAPS_GLOBALFOCUS (the game's flags are 0xe2), the sounds are muted
 * while the window lacks focus. They go on in silence: the positions move, a loop keeps its place,
 * and GetStatus tells the truth, so the commentary keeps its pace.
 *
 * RECOMP_SOUND_LOG=1 (or RECOMP_TRACE) prints each load, play, stop and end of a sound on stderr. */
#include "win.h"
#include "options.h"

#include <SDL.h>
#include <math.h>
#include <stdlib.h>

#define DEVICE_RATE 44100
#define VOICES 32                  /* the game uses ten slots; a buffer stays until its Release */
#define DSERR_INVALIDPARAM 0x80070057u
#define DSERR_BADFORMAT 0x88780064u
#define DSBPLAY_LOOPING 1u
#define DSBSTATUS_PLAYING 1u
#define DSBSTATUS_LOOPING 4u

static uint32_t ds_vtable, buffer_vtable;

/* A buffer in flat memory: vtable, count, data +8, size +0xc, voice number + 1 at +0x10 (0: none). */
#define BUF_VOICE 0x10
#define BUF_SIZE 0x14

typedef struct {
    int used;
    uint32_t buffer;               /* flat address of the buffer */
    int src_rate, src_channels, src_bits, src_align;
    int16_t *pcm;                  /* device format, stereo; owned by the voice */
    uint32_t frames;               /* frames in pcm */
    uint32_t pos;                  /* the next frame to play; the audio thread moves it */
    int playing, looping;          /* the audio thread clears playing at the end */
    int32_t volume, pan;           /* hundredths of dB, as DirectSound */
    float gain_l, gain_r;
    /* For the log, main thread only. */
    char name[64];
    int started;                   /* 1 from Play to the reported end or stop */
    uint32_t start_ms;
} Voice;

static Voice voices[VOICES];
static SDL_AudioDeviceID device;
static int device_tried;
static int sound_log = -1;
static volatile int muted;         /* the window lacks focus */

static int logging(void)
{
    if (sound_log < 0) {
        const char *v = getenv("RECOMP_SOUND_LOG");
        sound_log = (v && *v && strcmp(v, "0")) || win_trace;
    }
    return sound_log;
}

#define SOUND_LOG(...) do { if (logging()) fprintf(stderr, __VA_ARGS__); } while (0)

/* The SDL audio callback: the sum of the voices that play, with their gains, cut to 16 bits. */
static void mix(void *user, Uint8 *stream, int len)
{
    (void)user;
    int16_t *out = (int16_t *)stream;
    int frames = len / 4;
    static float acc[2 * 8192];
    while (frames > 0) {
        int n = frames > 8192 ? 8192 : frames;
        memset(acc, 0, sizeof(float) * 2 * (size_t)n);
        for (int v = 0; v < VOICES; v++) {
            Voice *vo = &voices[v];
            if (!vo->used || !vo->playing || !vo->pcm || !vo->frames) continue;
            for (int i = 0; i < n; i++) {
                if (vo->pos >= vo->frames) {
                    if (!vo->looping) { vo->playing = 0; vo->pos = 0; break; }
                    vo->pos = 0;
                }
                acc[2 * i] += vo->pcm[2 * vo->pos] * vo->gain_l;
                acc[2 * i + 1] += vo->pcm[2 * vo->pos + 1] * vo->gain_r;
                vo->pos++;
            }
            /* A sound that ends exactly at the end of the block also ends now. */
            if (vo->playing && !vo->looping && vo->pos >= vo->frames) { vo->playing = 0; vo->pos = 0; }
        }
        for (int i = 0; i < 2 * n; i++) {
            float s = muted ? 0.f : acc[i];
            out[i] = (int16_t)(s > 32767.f ? 32767 : s < -32768.f ? -32768 : s);
        }
        out += 2 * n;
        frames -= n;
    }
}

/* Opens the device once, if the settings file allows sound. A failure leaves the game silent. */
static void open_device(void)
{
    if (device_tried) return;
    device_tried = 1;
    if (!options_bool(OPT_SOUND)) {
        SOUND_LOG("sound: off in the settings file\n");
        return;
    }
    /* An unattended run with no window also makes no noise, unless it names an audio driver. */
    const char *video = SDL_getenv("SDL_VIDEODRIVER");
    if (video && !strcmp(video, "dummy") && !SDL_getenv("SDL_AUDIODRIVER")) SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        fprintf(stderr, "cm0102_recomp: no sound: %s\n", SDL_GetError());
        return;
    }
    SDL_AudioSpec want, have;
    memset(&want, 0, sizeof want);
    want.freq = DEVICE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = mix;
    device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!device) {
        fprintf(stderr, "cm0102_recomp: no sound: %s\n", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return;
    }
    SOUND_LOG("sound: device open, driver %s\n", SDL_GetCurrentAudioDriver());
    SDL_PauseAudioDevice(device, 0);
}

void win_sound_focus(int focused)
{
    if (muted == !focused) return;
    muted = !focused;
    SOUND_LOG("sound: %s\n", focused ? "focus back, heard again" : "focus lost, muted");
}

static void lock(void) { if (device) SDL_LockAudioDevice(device); }
static void unlock(void) { if (device) SDL_UnlockAudioDevice(device); }

static Voice *voice_of(uint32_t buffer)
{
    uint32_t n = rd32(g_mem, buffer + BUF_VOICE);
    return n && n <= VOICES && voices[n - 1].buffer == buffer ? &voices[n - 1] : NULL;
}

/* Hundredths of dB to a gain: 0 is full, -10000 is silence. */
static float db_gain(int32_t hundredths)
{
    if (hundredths <= -10000) return 0.f;
    return powf(10.f, (float)hundredths / 2000.f);
}

/* DirectSound pan: a positive pan lowers the left side, a negative pan the right side. */
static void set_gains(Voice *v)
{
    float g = db_gain(v->volume);
    v->gain_l = g * (v->pan > 0 ? db_gain(-v->pan) : 1.f);
    v->gain_r = g * (v->pan < 0 ? db_gain(v->pan) : 1.f);
}

/* Logs the end of a sound that played to its end. Main thread. */
static void note_end(Voice *v)
{
    if (v->started && !v->playing) {
        SOUND_LOG("sound: end %s after %u ms\n", v->name, SDL_GetTicks() - v->start_ms);
        v->started = 0;
    }
}

/* GetStatus(&status) */
static void buf_GetStatus(void)
{
    uint32_t status = 0;
    Voice *v = voice_of(rt_arg(0));
    if (v) {
        lock();
        if (v->playing) status = DSBSTATUS_PLAYING | (v->looping ? DSBSTATUS_LOOPING : 0);
        unlock();
        note_end(v);
    }
    if (rt_arg(1)) wr32(g_mem, rt_arg(1), status);
    rt_ret_stdcall(0, 2);
}

/* GetCurrentPosition(&play, &write), in bytes of the buffer's own format. */
static void buf_GetCurrentPosition(void)
{
    uint32_t bytes = 0;
    Voice *v = voice_of(rt_arg(0));
    if (v && v->pcm) {
        lock();
        uint64_t src = (uint64_t)v->pos * (uint64_t)v->src_rate / DEVICE_RATE;
        unlock();
        bytes = (uint32_t)src * (uint32_t)v->src_align;
    }
    if (rt_arg(1)) wr32(g_mem, rt_arg(1), bytes);
    if (rt_arg(2)) wr32(g_mem, rt_arg(2), bytes);
    rt_ret_stdcall(0, 3);
}

/* Lock(offset, bytes, &p1, &n1, &p2, &n2, flags) */
static void buf_Lock(void)
{
    uint32_t self = rt_arg(0), offset = rt_arg(1), bytes = rt_arg(2);
    uint32_t size = rd32(g_mem, self + 0xc);
    if (offset > size) offset = size;
    if (bytes > size - offset || bytes == 0) bytes = size - offset;
    wr32(g_mem, rt_arg(3), rd32(g_mem, self + 8) + offset);
    wr32(g_mem, rt_arg(4), bytes);
    if (rt_arg(5)) wr32(g_mem, rt_arg(5), 0);
    if (rt_arg(6)) wr32(g_mem, rt_arg(6), 0);
    rt_ret_stdcall(0, 8);
}

/* Unlock(p1, n1, p2, n2): the voice converts the whole buffer to the device format. */
static void buf_Unlock(void)
{
    uint32_t self = rt_arg(0);
    Voice *v = voice_of(self);
    uint32_t data = rd32(g_mem, self + 8), size = rd32(g_mem, self + 0xc);
    if (v && data && size >= (uint32_t)v->src_align) {
        SDL_AudioFormat fmt = v->src_bits == 8 ? AUDIO_U8 : AUDIO_S16LSB;
        SDL_AudioStream *s = SDL_NewAudioStream(fmt, (Uint8)v->src_channels, v->src_rate, AUDIO_S16SYS, 2,
                                                DEVICE_RATE);
        int16_t *pcm = NULL;
        uint32_t frames = 0;
        if (s) {
            uint32_t whole = size - size % (uint32_t)v->src_align;
            if (!SDL_AudioStreamPut(s, g_mem + data, (int)whole) && !SDL_AudioStreamFlush(s)) {
                int avail = SDL_AudioStreamAvailable(s);
                pcm = avail > 0 ? malloc((size_t)avail) : NULL;
                if (pcm) frames = (uint32_t)SDL_AudioStreamGet(s, pcm, avail) / 4;
            }
            SDL_FreeAudioStream(s);
        }
        lock();
        int16_t *old = v->pcm;
        v->pcm = pcm;
        v->frames = frames;
        if (v->pos >= frames) v->pos = 0;
        unlock();
        free(old);
    }
    rt_ret_stdcall(0, 5);
}

/* Play(reserved, priority, flags) */
static void buf_Play(void)
{
    Voice *v = voice_of(rt_arg(0));
    if (v) {
        lock();
        v->looping = (rt_arg(3) & DSBPLAY_LOOPING) != 0;
        v->playing = v->pcm && v->frames;
        unlock();
        if (v->playing) {
            SOUND_LOG("sound: play %s%s, %u ms, volume %d, pan %d\n", v->name, v->looping ? " (loop)" : "",
                      (uint32_t)((uint64_t)v->frames * 1000 / DEVICE_RATE), v->volume, v->pan);
            v->started = 1;
            v->start_ms = SDL_GetTicks();
        }
    }
    rt_ret_stdcall(0, 4);
}

/* SetCurrentPosition(offset in bytes of the buffer's format) */
static void buf_SetCurrentPosition(void)
{
    Voice *v = voice_of(rt_arg(0));
    if (v && v->src_rate) {
        uint64_t frame = rt_arg(1) / (uint32_t)v->src_align;
        lock();
        v->pos = (uint32_t)(frame * DEVICE_RATE / (uint64_t)v->src_rate);
        if (v->pos >= v->frames) v->pos = 0;
        unlock();
    }
    rt_ret_stdcall(0, 2);
}

/* SetVolume(hundredths of dB, -10000..0). Play at 0x84cbc0 first sends 10000: DirectSound refuses it
 * and keeps the volume. */
static void buf_SetVolume(void)
{
    Voice *v = voice_of(rt_arg(0));
    int32_t vol = (int32_t)rt_arg(1);
    if (vol > 0 || vol < -10000) { rt_ret_stdcall(DSERR_INVALIDPARAM, 2); return; }
    if (v) {
        lock();
        v->volume = vol;
        set_gains(v);
        unlock();
    }
    rt_ret_stdcall(0, 2);
}

/* SetPan(-10000 left .. 10000 right) */
static void buf_SetPan(void)
{
    Voice *v = voice_of(rt_arg(0));
    int32_t pan = (int32_t)rt_arg(1);
    if (pan > 10000 || pan < -10000) { rt_ret_stdcall(DSERR_INVALIDPARAM, 2); return; }
    if (v) {
        lock();
        v->pan = pan;
        set_gains(v);
        unlock();
    }
    rt_ret_stdcall(0, 2);
}

/* Stop keeps the position, as DirectSound does. */
static void buf_Stop(void)
{
    Voice *v = voice_of(rt_arg(0));
    if (v) {
        lock();
        int was = v->playing;
        v->playing = 0;
        unlock();
        if (was) {
            SOUND_LOG("sound: stop %s after %u ms\n", v->name, SDL_GetTicks() - v->start_ms);
            v->started = 0;
        } else {
            note_end(v);
        }
    }
    rt_ret_stdcall(0, 1);
}

/* The buffer's memory goes back to the heap on its last Release, and its voice becomes free. */
static void buf_Release(void)
{
    uint32_t self = rt_arg(0);
    uint32_t n = rd32(g_mem, self + 4);
    if (n) n--;
    wr32(g_mem, self + 4, n);
    if (!n) {
        Voice *v = voice_of(self);
        if (v) {
            note_end(v);
            lock();
            int16_t *pcm = v->pcm;
            memset(v, 0, sizeof *v);
            unlock();
            free(pcm);
            wr32(g_mem, self + BUF_VOICE, 0);
        }
        if (rd32(g_mem, self + 8)) {
            rt_free(rd32(g_mem, self + 8));
            wr32(g_mem, self + 8, 0);
        }
    }
    rt_ret_stdcall(n, 1);
}

static const WinMethod buffer_methods[] = {
    {0x04, "AddRef", win_com_addref}, {0x08, "Release", buf_Release},
    {0x10, "GetCurrentPosition", buf_GetCurrentPosition}, {0x24, "GetStatus", buf_GetStatus},
    {0x2c, "Lock", buf_Lock}, {0x30, "Play", buf_Play}, {0x34, "SetCurrentPosition", buf_SetCurrentPosition},
    {0x3c, "SetVolume", buf_SetVolume}, {0x40, "SetPan", buf_SetPan}, {0x48, "Stop", buf_Stop},
    {0x4c, "Unlock", buf_Unlock}, {0, NULL, NULL},
};

/* CreateSoundBuffer(desc, &buffer, outer). DSBUFFERDESC: size, flags, bytes +8, reserved, format +0x10.
 * WAVEFORMATEX: tag, channels +2, rate +4, bytes per second +8, block align +0xc, bits +0xe. */
static void ds_CreateSoundBuffer(void)
{
    uint32_t desc = rt_arg(1), out = rt_arg(2);
    uint32_t bytes = rd32(g_mem, desc + 8), wf = rd32(g_mem, desc + 0x10);
    int tag = 0, channels = 0, rate = 0, bits = 0;
    if (wf) {
        tag = rd16(g_mem, wf);
        channels = rd16(g_mem, wf + 2);
        rate = (int)rd32(g_mem, wf + 4);
        bits = rd16(g_mem, wf + 0xe);
    }
    if (wf && (tag != 1 || channels < 1 || channels > 2 || (bits != 8 && bits != 16) || rate < 1000 ||
               rate > 192000)) {
        SOUND_LOG("sound: format not supported: tag %d, %d channels, %d Hz, %d bits\n", tag, channels, rate,
                  bits);
        wr32(g_mem, out, 0);
        rt_ret_stdcall(DSERR_BADFORMAT, 4);
        return;
    }
    uint32_t b = win_make_object(buffer_vtable, BUF_SIZE);
    wr32(g_mem, b + 8, bytes ? rt_alloc_zero(bytes) : 0);
    wr32(g_mem, b + 0xc, bytes);
    if (device && wf) {
        for (int i = 0; i < VOICES; i++) {
            if (voices[i].used) continue;
            Voice *v = &voices[i];
            lock();
            memset(v, 0, sizeof *v);
            v->used = 1;
            v->buffer = b;
            v->src_rate = rate;
            v->src_channels = channels;
            v->src_bits = bits;
            v->src_align = channels * bits / 8;
            set_gains(v);
            unlock();
            snprintf(v->name, sizeof v->name, "(buffer %#x)", b);
            wr32(g_mem, b + BUF_VOICE, (uint32_t)i + 1);
            break;
        }
    }
    wr32(g_mem, out, b);
    rt_ret_stdcall(0, 4);
}

static void ds_SetCooperativeLevel(void) { rt_ret_stdcall(0, 3); }

static const WinMethod ds_methods[] = {
    {0x04, "AddRef", win_com_addref}, {0x08, "Release", win_com_release},
    {0x0c, "CreateSoundBuffer", ds_CreateSoundBuffer}, {0x18, "SetCooperativeLevel", ds_SetCooperativeLevel},
    {0, NULL, NULL},
};

/* DirectSoundCreate(guid, &object, outer), ordinal 1 of dsound.dll. */
RT_IMPORT(DSOUND_ord1)
{
    if (!ds_vtable) {
        ds_vtable = win_make_vtable("IDirectSound", 11, ds_methods);
        buffer_vtable = win_make_vtable("IDirectSoundBuffer", 21, buffer_methods);
    }
    open_device();
    wr32(g_mem, rt_arg(1), win_make_object(ds_vtable, 16));
    rt_ret_stdcall(0, 3);
}

void o_0084cd10(void);

/* The load of a WAV file into slot n of the sound object, thiscall (char slot, char *path): 1 when
 * the slot has a new buffer. Evidence for the loader: docs/research/2026-10-14-import-inventory.md
 * and the columns-and-colours inventory (0x84cd10). The argument layout is an inference. The wrapper only gives the buffer's voice the file name for the log. */
RT_WRAP(0084cd10, sound_load)
{
    uint32_t self = cpu.ecx;
    int slot = (int8_t)(rt_arg(0) & 0xff);
    char path[64] = "";
    if (rt_arg(1)) snprintf(path, sizeof path, "%s", win_str(rt_arg(1)));
    o_0084cd10();
    if (slot < 0 || slot >= 10) return;
    if (!cpu.eax) {
        if (path[0]) SOUND_LOG("sound: cannot load %s\n", path);
        return;
    }
    uint32_t buffer = rd32(g_mem, self + 4 * (uint32_t)slot);
    Voice *v = buffer ? voice_of(buffer) : NULL;
    if (v) {
        const char *base = strrchr(path, '\\');
        snprintf(v->name, sizeof v->name, "%s", base ? base + 1 : path);
    }
}
