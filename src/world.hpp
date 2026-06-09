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
    void fill(Vector3 a, Vector3 b, BlockType block_type);
    void replace(BlockType a, BlockType b);

    BlockType get_at(int x, int y, int z);
    BlockType get_at(Vector3 vec);

    void set_at(int x, int y, int z, BlockType type);
    void set_at(Vector3 vec, BlockType type);

    bool is_empty(int x, int y, int z);
    bool is_empty(Vector3 vec);

    Cube block_cube(int x, int y, int z);
    Cube block_cube(Vector3 pos);

    bool on_map(int x, int y, int z);
    bool on_map(Vector3 vec);

    c4v genC4v(int x, int y, int z, int faceN);

    void gen_tree(int x, int y, int z);

private:
    void update_block(int x, int y, int z);

    BlockType m_blocks[CHUNK_COUNT][CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];

public: // it's easier to have it as public for now
    std::vector<FallingBlock> falling_blocks;
};

