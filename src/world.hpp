#pragma once

#include <vector>

#include <raylib.h>

#include "blocktypes.hpp"
#include "util.hpp"

// static const int maxChunks = 100, chunkSize = 32;
static const int CHUNK_COUNT = 1;
static const int CHUNK_SIZE = 64;

static BlockType blocks[CHUNK_COUNT][CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];

struct FallingBlock {
    Vector3 vel;
    Vector3 pos;
    BlockType type;
};

static std::vector<FallingBlock> falling_blocks;

int load_world(std::string fName, Cube &pCube);
int save_world(std::string fName, const Cube &pCube);

bool on_map(int x, int y, int z);
bool on_map(Vector3 vec);

BlockType get_at(int x, int y, int z);
BlockType get_at(Vector3 vec);

void set_at(int x, int y, int z, BlockType type);
void set_at(Vector3 vec, BlockType type);

bool is_empty(int x, int y, int z);
bool is_empty(Vector3 vec);

Cube blockCube(int x, int y, int z);
Cube blockCube(Vector3 pos);

c4v genC4v(int x, int y, int z, int faceN);

void gen_tree(int x, int y, int z);

void clear_world();
void world_fill(Vector3 a, Vector3 b, BlockType block_type);

