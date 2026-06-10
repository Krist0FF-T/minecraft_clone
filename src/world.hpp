#pragma once

#include <vector>

#include <raylib.h>

#include "blocktypes.hpp"
#include "util.hpp"

// static const int CHUNK_COUNT = 100, CHUNK_SIZE = 32;
static const int CHUNK_COUNT = 1;
static const int CHUNK_SIZE = 64;

struct FallingBlock {
    Vector3 vel;
    Vector3 pos;
    BlockType type;
};

class World {
public:
    World();
    int load(std::string fName, Cube &pCube);
    int save(std::string fName, const Cube &pCube);
    void update();
    void update_falling_blocks(float dt);

    void clear();
    void fill(Vector3i a, Vector3i b, BlockType block_type);
    void replace(BlockType a, BlockType b);

    BlockType get_at(Vector3i pos);
    void set_at(Vector3i pos, BlockType type);
    bool is_empty(Vector3i pos);
    Cube block_cube(Vector3i pos);

    bool on_map(Vector3i pos);

    c4v genC4v(Vector3i pos, int faceN);
    void gen_tree(Vector3i pos);

private:
    void update_block(Vector3i pos);

    BlockType m_blocks[CHUNK_COUNT][CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];

public: // it's easier to have it as public for now
    std::vector<FallingBlock> falling_blocks;
};

