#include "world.hpp"

#include <fstream>

#include "blocktypes.hpp"

World::World() {}

void World::update() {
    for (int y = 0; y < CHUNK_SIZE * CHUNK_COUNT; y++) {
    for (int x = 0; x < CHUNK_SIZE; x++) {
    for (int z = 0; x < CHUNK_SIZE; x++) {
        update_block({x, y, z});
    } } }

    for (size_t j = 0; j < falling_blocks.size(); j++) {
        FallingBlock &fb = falling_blocks[j];
        fb.vel.y -= 0.08f;
        fb.vel.y *= 0.98f;
        if (fb.pos.y < -200) {
            falling_blocks.erase(falling_blocks.begin() + j);
            j--;
        }
        // fb.pos += fb.vel; -- done elsewhere
    }
}

void World::update_block(Vector3i pos) {
    BlockType type = get_at(pos);

    if (type == BlockType::Air) {
        return;
    }

    switch (type) {

    case BlockType::Grass: {
        if (get_at({pos.x, pos.y + 1, pos.z}) != BlockType::Air) {
            set_at(pos, BlockType::Dirt);
        }

        break;
    }

    case BlockType::Dirt: {
        if (BlockType::Air != get_at({pos.x, pos.y + 1, pos.z})) {
            break;
        }

        int grass_neighbors = 0;

        for (int x2 = -1; x2 <= 1; x2++) {
        for (int y2 = -1; y2 <= 1; y2++) {
        for (int z2 = -1; z2 <= 1; z2++) {
            Vector3i pos2 = pos + Vector3i{x2, y2, z2};
            if (BlockType::Grass == get_at(pos2)) {
                grass_neighbors++;
            }
        }}}

        if (grass_neighbors > 0 && rand() % 10 == 0) {
            set_at(pos, BlockType::Grass);
        }

        break;
    }

    case BlockType::Sand: {
        if (get_at({pos.x, pos.y - 1, pos.z}) == BlockType::Air) {
            set_at(pos, BlockType::Air);

            falling_blocks.push_back((FallingBlock){
                .vel = {0.0f, 0.0f, 0.0f},
                .pos = pos.to_raylib(),
                .type = type
            });
        }
        break;
    }

    default: {
        break;
    }
    }
}

void World::update_falling_blocks(float dt) {
    for (size_t j = 0; j < falling_blocks.size(); j++) {
        FallingBlock &fb = falling_blocks[j];
        Vector3 delta = fb.vel * dt * 20.0f;
        if (get_at(Vector3i::from_raylib(fb.pos + delta)) == BlockType::Air) {
            fb.pos += delta;
        } else {
            if (fb.type == BlockType::WoolRed) {
                set_at(Vector3i::from_raylib(fb.pos + delta), BlockType::Air);
            } else {
                set_at(Vector3i::from_raylib(fb.pos), fb.type);
            }
            falling_blocks.erase(falling_blocks.begin() + j);
            j--;
        }
    }
}

bool World::on_map(Vector3i pos) {
    return (
        0 <= pos.x && pos.x < CHUNK_SIZE &&
        0 <= pos.y && pos.y < CHUNK_SIZE * CHUNK_COUNT &&
        0 <= pos.z && pos.z < CHUNK_SIZE
    );
}

int chunk_of(int y) {
    return (int)y / CHUNK_SIZE;
}

BlockType World::get_at(Vector3i pos) {
    if (on_map(pos)) {
        return m_blocks[chunk_of(pos.y)][pos.x][pos.y % CHUNK_SIZE][pos.z];
    }
    return BlockType::Air;
}

void World::set_at(Vector3i pos, BlockType type) {
    if (on_map(pos)) {
        m_blocks[chunk_of(pos.y)][pos.x][pos.y % CHUNK_SIZE][pos.z] = type;
    }
}

bool World::is_empty(Vector3i pos) {
    return get_at(pos) == BlockType::Air;
}

Cube World::block_cube(Vector3i pos) {
    return Cube{pos.to_raylib(), Vector3One()};
}

c4v World::genC4v(Vector3i pos, int faceN) {
    bool top, bottom, left, right, topright, bottomright, topleft, bottomleft;

    switch (faceN) {
    case face_top: {
        top = !is_empty(pos + Vector3i{0, 1, -1});
        bottom = !is_empty(pos + Vector3i{0, 1, 1});
        left = !is_empty(pos + Vector3i{-1, 1, 0});
        right = !is_empty(pos + Vector3i{1, 1, 0});

        topleft = !is_empty(pos + Vector3i{-1, 1, -1});
        topright = !is_empty(pos + Vector3i{+1, 1, -1});
        bottomleft = !is_empty(pos + Vector3i{-1, + 1, + 1});
        bottomright = !is_empty(pos + Vector3i{1, 1, 1});

        break;
    }

    case face_bottom: {
        top = !is_empty(pos + Vector3i{0, - 1, - 1});
        bottom = !is_empty(pos + Vector3i{0, - 1, + 1});
        left = !is_empty(pos + Vector3i{- 1, - 1, 0});
        right = !is_empty(pos + Vector3i{+ 1, - 1, 0});

        topleft = !is_empty(pos + Vector3i{- 1, - 1, - 1});
        topright = !is_empty(pos + Vector3i{+ 1, - 1, - 1});
        bottomleft = !is_empty(pos + Vector3i{- 1, - 1, + 1});
        bottomright = !is_empty(pos + Vector3i{+ 1, - 1, + 1});

        break;
    }

    case face_left: {

        top = !is_empty(pos + Vector3i{- 1, + 1, 0});
        bottom = !is_empty(pos + Vector3i{- 1, - 1, 0});

        left = !is_empty(pos + Vector3i{- 1, 0, - 1});
        right = !is_empty(pos + Vector3i{- 1, 0, + 1});

        topleft = !is_empty(pos + Vector3i{- 1, + 1, - 1});
        topright = !is_empty(pos + Vector3i{- 1, + 1, + 1});

        bottomleft = !is_empty(pos + Vector3i{- 1, - 1, - 1});
        bottomright = !is_empty(pos + Vector3i{- 1, - 1, + 1});

        break;
    }

    case face_right: {

        top = !is_empty(pos + Vector3i{+ 1, + 1, 0});
        bottom = !is_empty(pos + Vector3i{+ 1, - 1, 0});

        left = !is_empty(pos + Vector3i{+ 1, 0, - 1});
        right = !is_empty(pos + Vector3i{+ 1, 0, + 1});

        topleft = !is_empty(pos + Vector3i{+ 1, + 1, - 1});
        topright = !is_empty(pos + Vector3i{+ 1, + 1, + 1});

        bottomleft = !is_empty(pos + Vector3i{+ 1, - 1, - 1});
        bottomright = !is_empty(pos + Vector3i{+ 1, - 1, + 1});

        break;
    }

    // --
    case face_front: {

        top = !is_empty(pos + Vector3i{0, + 1, - 1});
        bottom = !is_empty(pos + Vector3i{0, - 1, - 1});

        left = !is_empty(pos + Vector3i{- 1, 0, - 1});
        right = !is_empty(pos + Vector3i{+ 1, 0, - 1});

        topleft = !is_empty(pos + Vector3i{- 1, + 1, - 1});
        topright = !is_empty(pos + Vector3i{+ 1, + 1, - 1});

        bottomleft = !is_empty(pos + Vector3i{- 1, - 1, - 1});
        bottomright = !is_empty(pos + Vector3i{+ 1, - 1, - 1});

        break;
    }

    // ++
    case face_back: {
        top = !is_empty(pos + Vector3i{0, + 1, + 1});
        bottom = !is_empty(pos + Vector3i{0, - 1, + 1});

        left = !is_empty(pos + Vector3i{- 1, 0, + 1});
        right = !is_empty(pos + Vector3i{+ 1, 0, + 1});

        topleft = !is_empty(pos + Vector3i{- 1, + 1, + 1});
        topright = !is_empty(pos + Vector3i{+ 1, + 1, + 1});

        bottomleft = !is_empty(pos + Vector3i{- 1, - 1, + 1});
        bottomright = !is_empty(pos + Vector3i{+ 1, - 1, + 1});

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

void World::gen_tree(Vector3i pos) {
    if (get_at(pos) == BlockType::Grass) {
        return;
    }

    const int width = 5;
    const int height = 6;
    const int length = 5;

    // 5: dirt
    // 3: green

    BlockType uBlocks[2] = {BlockType::Log, BlockType::Leaf};

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
                Vector3i pos2 = {pos.x + j - width / 2, pos.y + i, pos.z + k - length / 2};
                if (treeBlocks[ind] != -1 &&
                    !(treeBlocks[ind] == -2 && rand() % 2 == 0) &&
                    is_empty(pos2)
                ) {
                    BlockType type;
                    if (treeBlocks[ind] == -2) {
                        type = BlockType::Leaf;
                    } else {
                        type = uBlocks[treeBlocks[ind]];
                    }
                    set_at(pos2, type);
                }

                ind++;
            }
        }
    }
}

void World::clear() {
    this->fill(
        {0, 0, 0},
        {CHUNK_SIZE - 1, CHUNK_SIZE - 1, CHUNK_SIZE - 1},
        BlockType::Air
    );
}

int World::save(std::string fName, const Cube &pCube) {
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
                    reinterpret_cast<const char *>(m_blocks[i][x][y]),
                    sizeof(uint8_t) * CHUNK_SIZE);
            }
        }
    }

    output_data.close();
    return 0;
}

void World::fill(Vector3i a, Vector3i b, BlockType block_type) {
    Vector3i p1 { std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z) };
    Vector3i p2 { std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z) };

    int c, rY;

    for (int x = (int)p1.x; x < (int)p2.x + 1; x++) {
        for (int y = (int)p1.y; y < (int)p2.y + 1; y++) {
            c = (int)(y / CHUNK_SIZE);
            rY = (y % CHUNK_SIZE);
            for (int z = (int)p1.z; z < (int)p2.z + 1; z++) {
                m_blocks[c][x][rY][z] = block_type;
            }
        }
    }
}

void World::replace(BlockType a, BlockType b) {
    for (int y = 0; y < CHUNK_COUNT*CHUNK_SIZE; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int z = 0; z < CHUNK_SIZE; z++)  {
                Vector3i pos {x, y, z};
                if (get_at(pos) == (BlockType) a) {
                    set_at(pos, (BlockType) b);
                }
            }
        }
    }
}

int World::load(std::string fName, Cube &pCube) {
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

    for (int chunk = 0; chunk < CHUNK_COUNT; chunk++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                input_data.read(reinterpret_cast<char *>(m_blocks[chunk][x][y]),
                                sizeof(uint8_t) * CHUNK_SIZE);
            }
        }
    }

    input_data.close();
    return 0;
}

