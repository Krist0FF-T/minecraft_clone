#pragma once

#include <fstream>
#include <vector>

#include <raylib.h>
#include <raymath.h>

#include "blocktypes.hpp"
#include "util.hpp"

// static const int maxChunks = 100, chunkSize = 32;
static const int CHUNK_COUNT = 1;
static const int CHUNK_SIZE = 64;

static const bool devMode = true;
static const bool trees = true;
static const bool flat = false;

static Vector3 tp_point;

static char blocks[CHUNK_COUNT][CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];

struct FallingBlock {
    Vector3 vel;
    Vector3 pos;
    int type;
};

static std::vector<FallingBlock> falling_blocks;

bool on_map(int x, int y, int z) {
    return (0 <= x && x < CHUNK_SIZE && 0 <= y &&
            y < CHUNK_SIZE * CHUNK_COUNT && 0 <= z && z < CHUNK_SIZE);
}

bool on_map(Vector3 vec) {
    return on_map((int)vec.x, (int)vec.y, (int)vec.z);
}

int chunk_of(int y) {
    return (int)y / CHUNK_SIZE;
}

int get_at(int x, int y, int z) {
    if (on_map(x, y, z)) {
        return blocks[chunk_of(y)][x][y % CHUNK_SIZE][z];
    }
    return bt_air;
}

int get_at(Vector3 vec) {
    return get_at((int)vec.x, (int)vec.y, (int)vec.z);
}

void set_at(int x, int y, int z, char type) {
    if (on_map(x, y, z)) {
        blocks[chunk_of(y)][x][y % CHUNK_SIZE][z] = type;
    }
}

void set_at(Vector3 vec, int type) {
    set_at((int)vec.x, (int)vec.y, (int)vec.z, type);
}

bool is_empty(int x, int y, int z) {
    return get_at(x, y, z) == bt_air;
}

bool is_empty(Vector3 vec) {
    return is_empty((int)vec.x, (int)vec.y, (int)vec.z);
}

Cube blockCube(int x, int y, int z) {
    return Cube{{(float)x, (float)y, (float)z}, Vector3One()};
}

Cube blockCube(Vector3 pos) {
    return Cube{pos, Vector3One()};
}

c4v genC4v(int x, int y, int z, int faceN) {
    bool top, bottom, left, right, topright, bottomright, topleft, bottomleft;

    switch (faceN) {
    case face_top: {
        top = !is_empty(x, y + 1, z - 1);
        bottom = !is_empty(x, y + 1, z + 1);
        left = !is_empty(x - 1, y + 1, z);
        right = !is_empty(x + 1, y + 1, z);

        topleft = !is_empty(x - 1, y + 1, z - 1);
        topright = !is_empty(x + 1, y + 1, z - 1);
        bottomleft = !is_empty(x - 1, y + 1, z + 1);
        bottomright = !is_empty(x + 1, y + 1, z + 1);

        break;
    }

    case face_bottom: {
        top = !is_empty(x, y - 1, z - 1);
        bottom = !is_empty(x, y - 1, z + 1);
        left = !is_empty(x - 1, y - 1, z);
        right = !is_empty(x + 1, y - 1, z);

        topleft = !is_empty(x - 1, y - 1, z - 1);
        topright = !is_empty(x + 1, y - 1, z - 1);
        bottomleft = !is_empty(x - 1, y - 1, z + 1);
        bottomright = !is_empty(x + 1, y - 1, z + 1);

        break;
    }

    case face_left: {

        top = !is_empty(x - 1, y + 1, z);
        bottom = !is_empty(x - 1, y - 1, z);

        left = !is_empty(x - 1, y, z - 1);
        right = !is_empty(x - 1, y, z + 1);

        topleft = !is_empty(x - 1, y + 1, z - 1);
        topright = !is_empty(x - 1, y + 1, z + 1);

        bottomleft = !is_empty(x - 1, y - 1, z - 1);
        bottomright = !is_empty(x - 1, y - 1, z + 1);

        break;
    }

    case face_right: {

        top = !is_empty(x + 1, y + 1, z);
        bottom = !is_empty(x + 1, y - 1, z);

        left = !is_empty(x + 1, y, z - 1);
        right = !is_empty(x + 1, y, z + 1);

        topleft = !is_empty(x + 1, y + 1, z - 1);
        topright = !is_empty(x + 1, y + 1, z + 1);

        bottomleft = !is_empty(x + 1, y - 1, z - 1);
        bottomright = !is_empty(x + 1, y - 1, z + 1);

        break;
    }

    // Z --
    case face_front: {

        top = !is_empty(x, y + 1, z - 1);
        bottom = !is_empty(x, y - 1, z - 1);

        left = !is_empty(x - 1, y, z - 1);
        right = !is_empty(x + 1, y, z - 1);

        topleft = !is_empty(x - 1, y + 1, z - 1);
        topright = !is_empty(x + 1, y + 1, z - 1);

        bottomleft = !is_empty(x - 1, y - 1, z - 1);
        bottomright = !is_empty(x + 1, y - 1, z - 1);

        break;
    }

    // Z ++
    case face_back: {
        top = !is_empty(x, y + 1, z + 1);
        bottom = !is_empty(x, y - 1, z + 1);

        left = !is_empty(x - 1, y, z + 1);
        right = !is_empty(x + 1, y, z + 1);

        topleft = !is_empty(x - 1, y + 1, z + 1);
        topright = !is_empty(x + 1, y + 1, z + 1);

        bottomleft = !is_empty(x - 1, y - 1, z + 1);
        bottomright = !is_empty(x + 1, y - 1, z + 1);

        break;
    }

    default: {
        top = false;
        bottom = false;
        left = false;
        right = false;
        topright = false;
        bottomright = false;
        topleft = false;
        bottomleft = false;
        break;
    }
    }

    float level = 0.24f;

    return (c4v){
        .tl = 1.0f - (top + left + topleft) * level,
        .bl = 1.0f - (top + right + topright) * level,
        .tr = 1.0f - (bottom + right + bottomright) * level,
        .br = 1.0f - (bottom + left + bottomleft) * level,
    };
}

void gen_tree(int x, int y, int z) {
    if (get_at(x, y, z) == bt_grass) {
        return;
    }

    const int width = 5;
    const int height = 6;
    const int length = 5;

    // 5: dirt
    // 3: green

    int uBlocks[2] = {bt_log, bt_leaf};

    int treeBlocks[width * height * length] = {
        /*
        -2,  8, -2,
        8,   8,  8,
        -1, -1, -1,

        8,  8,  8,
        8,  7,  8,
        -1, 7, -1,

        -2, 8, -2,
        8,  8,  8,
        -1, -1, -1,
        */

        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

        -1, -1, 1,  -1, -1, -1, -2, 1,  -2, -1, 1,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

        -1, 1,  1,  1,  -1, -1, 1,  1,  1,  -1, 1,  1,  0,  1,  1,
        1,  1,  0,  1,  1,  -1, -1, 0,  -1, -1, -1, -1, 0,  -1, -1,

        -1, -1, 1,  -1, -1, -1, -2, 1,  -2, -1, 1,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

    };

    int ind = 0;

    for (int j = 0; j < width; j++) {
        for (int i = height - 1; i >= 0; i--) {
            for (int k = 0; k < length; k++) {
                if (treeBlocks[ind] != -1 &&
                    !(treeBlocks[ind] == -2 && rand() % 2 == 0) &&
                    is_empty(x + j - width / 2, y + i, z + k - length / 2)) {
                    int type;
                    if (treeBlocks[ind] == -2) {
                        type = bt_leaf;
                    } else {
                        type = uBlocks[treeBlocks[ind]];
                    }
                    set_at(x + j - width / 2, y + i, z + k - length / 2,
                           type); // treeBlocks[x*l*h + y*w + z]
                }

                ind++;
            }
        }
    }
}

void clear_world() {
    for (int i = 0; i < CHUNK_COUNT; i++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    blocks[i][x][y][z] = -1;
                }
            }
        }
    }
}

int save_world(std::string fName, const Cube &pCube) {
    std::ofstream output_data;
    output_data.open("worlds/" + fName + ".save");
    if (!output_data) {
        return -1;
    }

    int16_t pos[3]{
        (int16_t)(pCube.pos.x * 10.0f),
        (int16_t)(pCube.pos.y * 10.0f),
        (int16_t)(pCube.pos.z * 10.0f),
    };

    output_data.write(reinterpret_cast<const char *>(pos), sizeof(int16_t) * 3);

    for (int i = 0; i < CHUNK_COUNT; i++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                output_data.write(
                    reinterpret_cast<const char *>(blocks[i][x][y]),
                    sizeof(char) * CHUNK_SIZE);
            }
        }
    }

    output_data.close();
    return 0;
}

int load_world(std::string fName, Cube &pCube) {
    std::ifstream input_data;
    input_data.open("worlds/" + fName + ".save");

    if (!input_data) {
        return -1;
    }

    falling_blocks.clear();
    int16_t pos[3];
    input_data.read(reinterpret_cast<char *>(pos), sizeof(int16_t) * 3);
    pCube.pos.x = (float)((int)pos[0] * 0.1f);
    pCube.pos.y = (float)((int)pos[1] * 0.1f);
    pCube.pos.z = (float)((int)pos[2] * 0.1f);
    tp_point = pCube.pos;

    for (int chunk = 0; chunk < CHUNK_COUNT; chunk++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                input_data.read(reinterpret_cast<char *>(blocks[chunk][x][y]),
                                sizeof(char) * CHUNK_SIZE);
            }
        }
    }

    input_data.close();
    return 0;
}
