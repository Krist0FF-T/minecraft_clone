#include "world.hpp"

#include <fstream>

#include <raylib.h>
#include <rlgl.h>

#include "blocktypes.hpp"
#include "draw.hpp"
#include "src/util.hpp"

World::World() {
    clear();
    fill({-16, 0, -16}, {15, 0, 15}, BlockType::Grass);
}

void World::update() {
    for (auto it = m_chunks.begin(); it != m_chunks.end(); it++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            update_block(it->first*CHUNK_SIZE + Vector3i {x, y, z});
        } } }
    }

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
    return m_chunks.find(pos / CHUNK_SIZE) != m_chunks.end();
}

BlockType World::get_at(Vector3i pos) {
    if (!on_map(pos)) {
        return BlockType::Air;
    }

    return m_chunks[pos / CHUNK_SIZE][pos % CHUNK_SIZE];
}

void World::set_at(Vector3i pos, BlockType type) {
    if (!on_map(pos)) {
        m_chunks[pos / CHUNK_SIZE] = Chunk();
    }

    m_chunks[pos / CHUNK_SIZE][pos % CHUNK_SIZE] = type;
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
    m_chunks.clear();
    falling_blocks.clear();
}

void World::fill(Vector3i a, Vector3i b, BlockType block_type) {
    Vector3i p1 { std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z) };
    Vector3i p2 { std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z) };

    for (int x = p1.x; x <= p2.x; x++) {
    for (int y = p1.y; y <= p2.y; y++) {
    for (int z = p1.z; z <= p2.z; z++) {
        set_at({x, y, z}, block_type);
    } } }
}

void World::replace(BlockType a, BlockType b) {
    for (auto it = m_chunks.begin(); it != m_chunks.end(); it++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++)  {
            Vector3i pos = it->first*CHUNK_SIZE + Vector3i {x, y, z};
            if (get_at(pos) == (BlockType) a) {
                set_at(pos, (BlockType) b);
            }
        } } }
    }
}

int World::load(std::string file_name, Cube &player_cube) {
    std::ifstream input_data;
    input_data.open("worlds/" + file_name + ".save");

    if (!input_data) {
        return -1;
    }

    falling_blocks.clear();
    m_chunks.clear();

    printf("reading player pos\n");
    Vector3i player_pos;
    input_data.read(reinterpret_cast<char *>(&player_pos), sizeof(player_pos));

    printf("reading chunk count\n");
    uint32_t chunk_count;
    input_data.read(reinterpret_cast<char *>(&chunk_count), sizeof(chunk_count));
    printf("%i chunks\n", chunk_count);

    printf("reading chunks\n");
    for (int chunk_idx = 0; chunk_idx < chunk_count; chunk_idx++) {
        Vector3i chunk_pos;
        input_data.read(reinterpret_cast<char*>(&chunk_pos), sizeof(chunk_pos));
        printf("idx=%i, pos=(%i, %i, %i)\n", chunk_idx, chunk_pos.x, chunk_pos.y, chunk_pos.z);

        Chunk chunk;

        for (int x = 0; x < CHUNK_SIZE; x++) {
            printf("row %i\n", x);
            for (int y = 0; y < CHUNK_SIZE; y++) {
                size_t idx = x * CHUNK_SIZE*CHUNK_SIZE + y * CHUNK_SIZE;
                input_data.read(
                    reinterpret_cast<char *>(&chunk.blocks[idx]),
                    sizeof(BlockType) * CHUNK_SIZE
                );
            }
        }

        m_chunks[chunk_pos] = chunk;
    }

    input_data.close();
    return 0;
}

int World::save(std::string file_name, const Cube &player_cube) {
    std::ofstream output_data;
    output_data.open("worlds/" + file_name + ".save");
    if (!output_data) {
        return -1;
    }

    Vector3i player_pos = Vector3i::from_raylib(player_cube.pos);

    TraceLog(LOG_DEBUG, "saving world %s", file_name.c_str());

    TraceLog(LOG_DEBUG, "(saving) player pos x=%i, y=%i, z=%i", player_pos.x, player_pos.y, player_pos.z);
    output_data.write(reinterpret_cast<const char *>(&player_pos), sizeof(player_pos));

    std::uint32_t chunk_count = m_chunks.size();
    TraceLog(LOG_DEBUG, "(saving) chunk count %i", chunk_count);
    output_data.write(reinterpret_cast<const char*>(&chunk_count), sizeof(chunk_count));

    TraceLog(LOG_DEBUG, "(saving) writing %i chunks", chunk_count);
    for (auto it = m_chunks.begin(); it != m_chunks.end(); it++) {
        output_data.write(reinterpret_cast<const char*>(&it->first), sizeof(Vector3i));
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                size_t idx = x * CHUNK_SIZE * CHUNK_SIZE + y * CHUNK_SIZE;
                output_data.write(
                    reinterpret_cast<const char *>(&it->second.blocks[idx]),
                    sizeof(uint8_t) * CHUNK_SIZE
                );
            }
        }
    }
    output_data.close();

    TraceLog(LOG_DEBUG, "world saved");
    return 0;
}

void World::draw(const Camera& camera) {
    // rlCheckRenderBatchLimit((blockCount + fallingBlocks.size())*3);

    rlPushMatrix();
    rlBegin(RL_QUADS);
    for (FallingBlock& fb : falling_blocks) {
        Faces f{
            camera.position.x<fb.pos.x, camera.position.x> fb.pos.x,
            camera.position.y<fb.pos.y, camera.position.y> fb.pos.y,
            camera.position.z<fb.pos.z, camera.position.z> fb.pos.z,
        };

        drawCubeTextureFaces(fb.pos, f, fb.type);
    }
    rlEnd();
    rlPopMatrix();

    for (auto it = m_chunks.begin(); it != m_chunks.end(); it++) {
        Vector3 size = {CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE};
        DrawCubeWiresV((it->first*CHUNK_SIZE).to_raylib() + size/2 - Vector3Ones/2, size, BLACK);

        if (IsKeyDown(KEY_TAB))
            continue;

        rlPushMatrix();
        rlBegin(RL_QUADS);
        for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            draw_block(
                it->first*CHUNK_SIZE + Vector3i {x, y, z},
                camera
            );
        } } }
        rlEnd();
        rlPopMatrix();
    }

}

void World::draw_block(Vector3i pos, const Camera& camera) {
    if (is_empty(pos)) {
        return;
    }

    // printf("e\n");

    static const float b_top = 1.0f, // top
        b1 = 0.9f,                   // left, right
        b2 = 0.8f,                   // front, back
        b_bottom = 0.7f;             // bottom

    static int lastT = -1, t = 0;
    lastT = -1;

    Faces faces = {
        (camera.position.x < pos.x && is_empty(pos + Vector3i{-1, 0, 0})),
        (camera.position.x > pos.x && is_empty(pos + Vector3i{+1, 0, 0})),
        (camera.position.y < pos.y && is_empty(pos + Vector3i{0, -1, 0})),
        (camera.position.y > pos.y && is_empty(pos + Vector3i{0, +1, 0})),
        (camera.position.z < pos.z && is_empty(pos + Vector3i{0, 0, -1})),
        (camera.position.z > pos.z && is_empty(pos + Vector3i{0, 0, +1})),
    };

    if (!faces.back && !faces.front && !faces.left && !faces.right &&
        !faces.top && !faces.bottom) {
        return;
    }

    uint8_t type_idx = (uint8_t)get_at(pos);
    auto sides = blockData[type_idx].sides;

    // Top Face
    if (faces.top) {
        t = block_textures[sides[0]].id;
        if (lastT != t) {
            rlSetTexture(t);
            lastT = t;
        }
        c4v vc = genC4v(pos, face_top);

        rlNormal3f(0.0f, 1.0f, 0.0f);

        // Top Left of the Quad
        color4ubGF(vc.tl * b_top);
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(pos.x - 0.5f, pos.y + 0.5f, pos.z - 0.5f);

        // Bottom Left Of The Quad
        color4ubGF(vc.br * b_top);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(pos.x - 0.5f, pos.y + 0.5f, pos.z + 0.5f);

        // Bottom Right Of The Quad
        color4ubGF(vc.tr * b_top);
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.5f);

        // Top Right Of The Quad
        color4ubGF(vc.bl * b_top);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(pos.x + 0.5f, pos.y + 0.5f, pos.z - 0.5f);
    }

    // Right face
    t = block_textures[sides[1]].id;
    if (t != lastT &&
        (faces.right || faces.left || faces.back || faces.front)) {
        rlSetTexture(t);
        lastT = t;
    }

    if (faces.right) {
        rlNormal3f(1.0f, 0.0f, 0.0f);

        c4v vc = genC4v(pos, face_right);

        // Bottom Right Of The Texture and Quad
        color4ubGF(vc.br * b1);
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(pos.x + 0.5f, pos.y - 0.5f, pos.z - 0.5f);

        // Top Right Of The Texture and Quad
        color4ubGF(vc.tl * b1);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(pos.x + 0.5f, pos.y + 0.5f, pos.z - 0.5f);

        // Top Left Of The Texture and Quad
        color4ubGF(vc.bl * b1);
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.5f);

        // Bottom Left Of The Texture and Quad
        color4ubGF(vc.tr * b1);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(pos.x + 0.5f, pos.y - 0.5f, pos.z + 0.5f);
    }

    // Left Face
    if (faces.left) {
        rlNormal3f(-1.0f, 0.0f, 0.0f);

        c4v vc = genC4v(pos, face_left);

        // Bottom Left Of The Texture and Quad
        color4ubGF(vc.br * b1);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(pos.x - 0.5f, pos.y - 0.5f, pos.z - 0.5f);

        // Bottom Right Of The Texture and Quad
        color4ubGF(vc.tr * b1);
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(pos.x - 0.5f, pos.y - 0.5f, pos.z + 0.5f);

        // Top Right Of The Texture and Quad
        color4ubGF(vc.bl * b1);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(pos.x - 0.5f, pos.y + 0.5f, pos.z + 0.5f);

        // Top Left Of The Texture and Quad
        color4ubGF(vc.tl * b1);
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(pos.x - 0.5f, pos.y + 0.5f, pos.z - 0.5f);
    }

    // Front Face
    if (faces.back) {
        // color4ubG(c*b2, 255, 0);
        rlNormal3f(0.0f, 0.0f, 1.0f); // Normal Pointing Towards Viewer

        c4v vc = genC4v(pos, face_back);

        // Bottom Left Of The Texture and Quad
        color4ubGF(vc.br * b2);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(pos.x - 0.5f, pos.y - 0.5f, pos.z + 0.5f);

        // Bottom Right Of The Texture and Quad
        color4ubGF(vc.tr * b2);
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(pos.x + 0.5f, pos.y - 0.5f, pos.z + 0.5f);

        // Top Right Of The Texture and Quad
        color4ubGF(vc.bl * b2);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.5f);

        // Top Left Of The Texture and Quad
        color4ubGF(vc.tl * b2);
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(pos.x - 0.5f, pos.y + 0.5f, pos.z + 0.5f);
    }

    // Back Face
    if (faces.front) {
        // color4ubG(c*b2, 255, 0);
        // rlNormal3f(0.0f, 0.0f, - 1.0f);                  // Normal
        // Pointing Away From Viewer

        c4v vc = genC4v(pos, face_front);

        // Bottom Right Of The Texture and Quad
        color4ubGF(vc.br * b2);
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(pos.x - 0.5f, pos.y - 0.5f, pos.z - 0.5f);

        // Top Right Of The Texture and Quad
        color4ubGF(vc.tl * b2);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(pos.x - 0.5f, pos.y + 0.5f, pos.z - 0.5f);

        // Top Left Of The Texture and Quad
        color4ubGF(vc.bl * b2);
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(pos.x + 0.5f, pos.y + 0.5f, pos.z - 0.5f);

        // Bottom Left Of The Texture and Quad
        color4ubGF(vc.tr * b2);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(pos.x + 0.5f, pos.y - 0.5f, pos.z - 0.5f);
    }

    // Bottom Face
    if (faces.bottom) {
        t = block_textures[sides[2]].id;
        if (lastT != t) {
            rlSetTexture(t);
            lastT = t;
        }

        rlNormal3f(0.0f, -1.0f, 0.0f);

        // Top Right Of The Texture and Quad
        c4v vc = genC4v(pos, face_bottom);
        color4ubGF(vc.tl * b_bottom);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(pos.x - 0.5f, pos.y - 0.5f, pos.z - 0.5f);

        // Top Left Of The Texture and Quad
        color4ubGF(vc.bl * b_bottom);
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(pos.x + 0.5f, pos.y - 0.5f, pos.z - 0.5f);

        // Bottom Left Of The Texture and Quad
        color4ubGF(vc.tr * b_bottom);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(pos.x + 0.5f, pos.y - 0.5f, pos.z + 0.5f);

        // Bottom Right Of The Texture and Quad
        color4ubGF(vc.br * b_bottom);
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(pos.x - 0.5f, pos.y - 0.5f, pos.z + 0.5f);
    }
}
