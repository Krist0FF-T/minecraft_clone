#pragma once

#include <raylib.h>

static const int N_SOUNDS = 2;
static Sound sounds[N_SOUNDS];

static const char *soundNames[N_SOUNDS]{"place", "break"};

enum SOUND { SOUND_PLACE = 0, SOUND_BREAK };

void init_sounds() {
    for (int i = 0; i < N_SOUNDS; i++) {
        sounds[i] = LoadSound(TextFormat("sound/%s.wav", soundNames[i]));
    }
}

void deinit_sounds() {
    for (int i = 0; i < N_SOUNDS; i++) {
        UnloadSound(sounds[i]);
    }
}
