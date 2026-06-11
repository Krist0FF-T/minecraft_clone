#pragma once

#include <unordered_map>
#include <vector>

#include <raylib.h>

#include "blocktypes.hpp"
#include "util.hpp"

static const int CHUNK_SIZE = 8;

struct FallingBlock {
    Vector3 vel;
    Vector3 pos;
    BlockType type;
};

struct Chunk {
    std::array<BlockType, CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE> blocks;

    // TODO: mesh generation
    // Mesh mesh;
    // bool mesh_dirty;

    Chunk() {
        blocks.fill(BlockType::Air);
    }

    BlockType& operator[](Vector3i local) {
        return blocks[local.x * CHUNK_SIZE * CHUNK_SIZE + local.y * CHUNK_SIZE + local.z];
    }
};

class World {
public:
    World();

    void draw(const Camera& camera);
    void draw_block(Vector3i pos, const Camera& camera);

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

    void gen_tree(Vector3i pos);

private:
    c4v genC4v(Vector3i pos, int faceN);
    void update_block(Vector3i pos);

    std::unordered_map<Vector3i, Chunk, Vector3iHash> m_chunks;

public: // it's easier to have it as public for now
    std::vector<FallingBlock> falling_blocks;
};

