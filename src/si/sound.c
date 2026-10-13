#include "cm_types.h"
#include <SDL2/SDL.h>

static bool g_sound_ready = false;

void sound_init(void)
{
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        SDL_Log("Audio init failed: %s", SDL_GetError());
        return;
    }
    g_sound_ready = true;
}

void sound_shutdown(void)
{
    if (g_sound_ready) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        g_sound_ready = false;
    }
}

void sound_play_effect(const void *data, u32 size)
{
    (void)data;
    (void)size;
    /* TODO: implement WAV playback via SDL_AudioStream */
}

void sound_set_volume(s32 volume)
{
    (void)volume;
}
