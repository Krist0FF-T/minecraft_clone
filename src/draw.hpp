#pragma once

#include <raylib.h>

#include "blocktypes.hpp"

struct Faces {
    bool left;   // X -
    bool right;  // X +
    bool bottom; // Y -
    bool top;    // Y +
    bool front;  // Z -
    bool back;   // Z +
};

extern Texture2D block_textures[TEXTURE_COUNT];

void init_textures(const char *texture_pack);

void color4ubGF(float g);

void drawCubeTextureFaces(Vector3 position, Faces faces, BlockType type);

void drawKeyStroke(Vector2 pos, int key, float size);

void drawKeystrokes(Vector2 pos, float size);
