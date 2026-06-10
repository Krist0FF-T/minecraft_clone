
#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#define GLSL_VERSION 330

// own
#include "blocktypes.hpp"
#include "draw.hpp"
#include "util.hpp"
#include "world.hpp"

#define VERSION "0.0.6 (DEVMODE)"

static Font font;
static const char *fontFName = "fonts/retro.ttf";

World world;

static const Color BACKGROUND_COLOR {100, 180, 240, 255};

static const float PLAYER_WIDTH = 0.6f, PLAYER_HEIGHT = 1.8f;
static Cube player_cube{{0, 0, 0}, {PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_WIDTH}};

static const float DEF_PLAYER_SPEED = 5.612f;
static float player_speed = DEF_PLAYER_SPEED;
static float rot_x = 0.0f, rot_y = 90.0f;
static float vel_y;

static const float PLAYER_EYE = PLAYER_HEIGHT * 0.4f;

static const float DEF_FOV = 90.0f;
static float cur_fov = DEF_FOV;

static const float REACH_STEP = 0.01f;
static float reach = 5.0f;

static float dt;
static const float TICK_TIME = 0.05f;
static float tick_timer = 0.0f;

static BlockType cur_type = BlockType::Grass;
static int render_dist = 2;

static bool able_to_jump = false;
static bool flying = false;

static bool fast_place = 0;
static bool smoothLighting = true;

static Camera3D camera;
static Vector3 camera_direction;

static Vector3i looking_at = {0, 0, 0};

static const int N_SOUNDS = 2;
static Sound sounds[N_SOUNDS];

static const char *soundNames[N_SOUNDS]{"place", "break"};

enum SOUND { SOUND_PLACE = 0, SOUND_BREAK };

static std::vector<std::string> notifications;
static float notification_cooldown = 0.0f;

void notify(std::string txt) {
    if (notifications.size() == 0) {
        notification_cooldown = 2.0f;
    }
    notifications.push_back(txt);
}

void drawCrosshair(Color color) {
    int chSize = GetScreenHeight() / 72;
    int chLineWidth = GetScreenHeight() / 270;

    DrawLineEx(
        {(float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() / 2 - chSize},
        {(float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() / 2 + chSize},
        chLineWidth, color);

    DrawLineEx(
        {(float)GetScreenWidth() * 0.5f - chSize, (float)GetScreenHeight() / 2},
        {(float)GetScreenWidth() * 0.5f + chSize, (float)GetScreenHeight() / 2},
        chLineWidth, color);
}

std::string guiInputTxt(std::string displayText);

// declaration of pause function
void pause();
void blockSelectionMenu();

bool blockCollidePlayer(int x, int y, int z, Vector3 p) {
    return (p.x > (float)(x - PLAYER_WIDTH) &&
            p.x < (float)(x + PLAYER_WIDTH) &&
            p.y > (float)(y - PLAYER_HEIGHT * 0.25f) &&
            p.y < (float)(y + PLAYER_HEIGHT * 1.75f) &&
            p.z > (float)(z - PLAYER_WIDTH / 2) &&
            p.z < (float)(z + PLAYER_WIDTH / 2));
}

void place_player() {
    bool placed = false;

    while (!placed) {
        int x = rand() % 32;
        int z = rand() % 32;

        for (int y = CHUNK_COUNT * CHUNK_SIZE - 1; y > -1 && !placed; y--) {
            placed = true;
            for (int a = 1; a < 3 && placed; a++) {
                if (!world.is_empty({x, y + a, z})) {
                    placed = false;
                }
            }

            if (world.is_empty({x, y, z})) {
                placed = false;
            }

            if (placed) {
                player_cube.pos.x = x;
                player_cube.pos.y = y + PLAYER_HEIGHT;
                player_cube.pos.z = z;
            }
        }
    }
}

void command_fill() {
    Vector3i a, b;
    BlockType type;

    try {
        std::vector<std::string> inputPos1 =
            split_str(guiInputTxt("pos1: [x] [y] [z]"), ' ');
        if (inputPos1.size() != 3) {
            notify("correct format: [x] [y] [z]");
            return;
        }

        std::vector<std::string> inputPos2 =
            split_str(guiInputTxt("pos2: [x] [y] [z]"), ' ');
        if (inputPos2.size() != 3) {
            notify("correct format: [x] [y] [z]");
            return;
        }

        a.x = std::stoi(inputPos1[0]);
        a.y = std::stoi(inputPos1[1]);
        a.z = std::stoi(inputPos1[2]);

        b.x = std::stoi(inputPos2[0]);
        b.y = std::stoi(inputPos2[1]);
        b.z = std::stoi(inputPos2[2]);

        int type_idx = std::stoi(guiInputTxt("type"));

        if (type_idx < -1 || type_idx > BLOCK_TYPE_COUNT - 1) {
            notify("Type out of range.");
            return;
        }
        type = (BlockType) type_idx;
    } catch (const std::invalid_argument &e) {
        notify(e.what());
        return;
    } catch (const std::out_of_range &e) {
        notify(e.what());
        return;
    }

    if (!world.on_map(a) || !world.on_map(b)) {
        notify("Not on map.");
        return;
    }

    world.fill(a, b, type);
}

void tick() {
    if (!flying) {
        vel_y -= 0.08f;
        vel_y *= 0.98f;
    }

    world.update();
}

void update() {
    // tick
    tick_timer += dt;
    while (tick_timer >= TICK_TIME) {
        tick_timer -= TICK_TIME;
        tick();
    }
    // ----

    notification_cooldown -= dt;
    if (notification_cooldown < 0.0f && notifications.size() > 0) {
        notifications.erase(notifications.begin());
        notification_cooldown = 1.0f;
    }

    cur_type = (BlockType) ((int)cur_type + GetMouseWheelMove());
    cur_type = (BlockType)std::min(std::max((int)cur_type, 0), BLOCK_TYPE_COUNT - 1);

    const bool ctrl = IsKeyDown(KEY_LEFT_CONTROL);

    bool forward = IsKeyDown(KEY_W), back = IsKeyDown(KEY_S),
         right = IsKeyDown(KEY_D), left = IsKeyDown(KEY_A),

         up = IsKeyDown(KEY_SPACE), down = IsKeyDown(KEY_LEFT_SHIFT);

    // move ===

    if (able_to_jump && up) {
        vel_y = 0.5081f;
        able_to_jump = false;
    }

    if (abs(vel_y) > 0.1f) {
        able_to_jump = false;
    }

    // don't let the player fall too far
    // if (pCube.pos.y < -10) {
    //   flying = true;
    // }

    if (flying) {
        vel_y = (up - down) * player_speed * TICK_TIME;
    }

    world.update_falling_blocks(dt);

    Vector3 player_pos_delta;
    player_pos_delta.x = (dCos(rot_x) * (forward - back) - dSin(rot_x) * (right - left));
    player_pos_delta.z = (dSin(rot_x) * (forward - back) + dCos(rot_x) * (right - left));
    player_pos_delta = Vector3Normalize(player_pos_delta) * player_speed * dt;

    player_pos_delta.y = vel_y * dt * 20.0f; // velY*dt;

    int testW = (int)ceilf(player_pos_delta.x) + 1;
    int testH = (int)ceilf(player_pos_delta.y) + 1;
    int testL = (int)ceilf(player_pos_delta.z) + 1;

    for (int x = (int)player_cube.getTLF().x - 1;
         x < (int)std::ceilf(player_cube.getBRB().x) + 2 + testW + 1; x++) {
        for (int y = (int)player_cube.getTLF().y - 1;
             y < (int)std::ceilf(player_cube.getBRB().y) + 2 + testH + 1; y++) {
            for (int z = (int)player_cube.getTLF().z - 1;
                 z < (int)std::ceilf(player_cube.getBRB().z) + 2 + testL + 1;
                 z++) {
                Vector3i pos = {x, y, z};
                if (flying || world.is_empty(pos)) {
                    continue;
                }

                Cube cube = world.block_cube(pos);

                if (player_cube.movedCopy({player_pos_delta.x, 0, 0})
                        .collide(cube)) {
                    player_pos_delta.x = 0;
                }

                if (player_cube.movedCopy({0, player_pos_delta.y, 0})
                        .collide(cube)) {
                    player_pos_delta.y = 0;
                    vel_y = 0;

                    if (player_cube.getTLF().y + PLAYER_HEIGHT > (float)y) {
                        able_to_jump = true;
                        if (world.get_at(pos) == BlockType::Slime) {
                            vel_y = 4;
                        }
                    }
                }

                if (player_cube.movedCopy({0, 0, player_pos_delta.z})
                        .collide(cube)) {
                    player_pos_delta.z = 0;
                }
            }
        }
    }

    player_cube.pos += player_pos_delta;

    camera.position = {player_cube.pos.x, player_cube.pos.y + PLAYER_EYE,
                       player_cube.pos.z};

    // view up & down

    const float rot_speed = IsKeyDown(KEY_LEFT_ALT) ? 8.0f : 1.25f;

    rot_x += GetMouseDelta().x * 0.2f;
    rot_x +=
        (IsKeyDown(KEY_L) - IsKeyDown(KEY_H)) * camera.fovy * rot_speed * dt;

    if (rot_x > 360) {
        rot_x -= 360;
    }
    if (rot_x < 0) {
        rot_x += 360;
    }

    rot_y += GetMouseDelta().y * 0.2f;
    rot_y +=
        (IsKeyDown(KEY_J) - IsKeyDown(KEY_K)) * camera.fovy * rot_speed * dt;
    rot_y = std::clamp(rot_y, 0.1f, 179.9f);

    camera_direction = {dSin(rot_y) * dCos(rot_x), dCos(rot_y), dSin(rot_y) * dSin(rot_x)};
    camera.target = camera.position + camera_direction;

    // Toggle fullscreen
    if (IsKeyPressed(KEY_F11)) {
        ToggleFullscreen();
    }

    // zoom
    cur_fov = IsKeyDown(KEY_C) ? (DEF_FOV * 0.3f) : DEF_FOV;
    camera.fovy += dt * 20 * (cur_fov - camera.fovy);

    Vector3i place_add {0, 0, 0};
    for (float cur_rad = 0; cur_rad < reach + REACH_STEP; cur_rad += REACH_STEP) {
        Vector3 look_at_f = camera.position + camera_direction * cur_rad;
        looking_at = Vector3i::from_raylib(look_at_f);

        if (world.is_empty(looking_at)) {
            looking_at.x = -1;
            continue;
        }

        // block found

        float dx = look_at_f.x - looking_at.x;
        float dy = look_at_f.y - looking_at.y;
        float dz = look_at_f.z - looking_at.z;

        float add = 0.5f - REACH_STEP;

        if (dx < -add) {
            place_add = {-1, 0, 0};
        } else if (dx > add) {
            place_add = {1, 0, 0};
        } else if (dy < -add) {
            place_add = {0, -1, 0};
        } else if (dy > add) {
            place_add = {0, 1, 0};
        } else if (dz < -add) {
            place_add = {0, 0, -1};
        } else if (dz > add) {
            place_add = {0, 0, 1};
        }

        break;
    }

    if (IsKeyPressed(KEY_P)) {
        fast_place = !fast_place;
    }

    Vector3i placeAt = looking_at + place_add;

    if ((IsKeyDown(KEY_X) || IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) &&
        !world.is_empty(looking_at)) {
        cur_type = world.get_at(looking_at);
    }

    bool action_break = (
        (fast_place ? IsKeyDown : IsKeyPressed)(KEY_U) ||
        (fast_place ? IsMouseButtonDown : IsMouseButtonPressed)(MOUSE_LEFT_BUTTON)
    );
    if (action_break &&
        !world.is_empty(looking_at) &&
        world.on_map(looking_at) && world.on_map(looking_at)) {
        world.set_at(looking_at, BlockType::Air);
        PlaySound(sounds[SOUND_BREAK]);
    }

    bool action_place = (
        (fast_place ? IsKeyDown : IsKeyPressed)(KEY_O) ||
        (fast_place ? IsMouseButtonDown : IsMouseButtonPressed)(MOUSE_RIGHT_BUTTON)
    );
    if (action_place && (flying || !player_cube.collide(world.block_cube(placeAt)))) {
        if (world.on_map(placeAt) && world.is_empty(placeAt)) {
            world.set_at(placeAt, cur_type);
            // fallingBlocks.push_back({0, {placeAt.x, placeAt.y+0.5f,
            // placeAt.z},
            world.set_at(placeAt, cur_type);
            PlaySound(sounds[SOUND_PLACE]);
        } else if (looking_at.x == -1) {
            Vector3 pos = player_cube.pos + player_cube.size / 2 + camera_direction * 2;

            world.falling_blocks.push_back((FallingBlock) {
                camera_direction * 2,
                pos,
                cur_type,
            });
        }
    }

    if (world.on_map(placeAt) && IsKeyPressed(KEY_I)) {
        world.gen_tree(placeAt);
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        pause();
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        pause();
    }

    if (IsKeyPressed(KEY_E)) {
        blockSelectionMenu();
    }

    if (IsKeyDown(KEY_Q) && looking_at.x != -1) {
        world.set_at(looking_at, cur_type);
    }

    if (IsKeyPressed(KEY_F2)) {
        take_screenshot();
    }

    // toggle flying
    if (!ctrl && IsKeyPressed(KEY_F)) {
        flying = !flying;
    }

    // save world
    if (ctrl && IsKeyPressed(KEY_S)) {
        std::string fName = guiInputTxt("save as");
        if (fName != "" && world.save(fName, player_cube) == 0) {
            printf("[+] worlds saved.\n");
            notify("World saved");
        } else {
            notify("Failed saving world");
            TraceLog(LOG_ERROR, "world NOT savééééd");
            // printf("[ERROR] world NOT saved.\n");
        }
    }

    // load world
    if (ctrl && IsKeyPressed(KEY_L)) {
        std::string fName = guiInputTxt("load from");
        if (fName != "" && world.load(fName, player_cube) == 0) {
            vel_y = 0;
            notify("World loaded");
            printf("[+] world loaded.\n");

        } else {
            notify("Failed loading world");
            printf("[ERROR] world NOT loaded.\n");
        }
    }

    // Toggle smooth lighting
    if (ctrl && IsKeyPressed(KEY_M)) {
        smoothLighting = !smoothLighting;
    }

    // clear world
    if (ctrl && IsKeyPressed(KEY_C)) {
        world.clear();
    }

    // change texture pack
    if (ctrl && IsKeyPressed(KEY_Z)) {
        std::string name = guiInputTxt("texture pack name");
        init_textures(name.c_str());
    }

    if (ctrl && IsKeyPressed(KEY_B)) {
        std::string command = guiInputTxt("command:");

        if (command == "fill") {
            command_fill();
        }

        else if (command == "replace") {
            try {
                BlockType a = (BlockType)std::stoi(guiInputTxt("from"));
                BlockType b = (BlockType)std::stoi(guiInputTxt("to"));
                world.replace(a, b);
            } catch (std::exception &e) {
                printf("[ERROR] %s", e.what());
                notify(TextFormat("[ERROR] %s", e.what()));
            }
        }

        else if (command == "tp") {
            try {
                std::vector<std::string> input =
                    split_str(guiInputTxt("teleport to: [x] [y] [z]"), ' ');
                if (input.size() == 3) {
                    player_cube.pos = {
                        (float)std::stoi(input[0]),
                        (float)std::stoi(input[1]),
                        (float)std::stoi(input[2]),
                    };
                } else {
                    notify("Enter 3 numbers please");
                }
            } catch (const std::invalid_argument &e) {
                notify(e.what());
            } catch (const std::out_of_range &e) {
                notify(e.what());
            }
        }

        else if (command == "reach") {
            std::string input = guiInputTxt("reach value");
            try {
                reach = std::stof(input);
            } catch (std::exception &e) {
                printf("[ERROR] %s\n", e.what());
                notify(TextFormat("[ERROR] %s", e.what()));
            }
        }

        else if (command == "speed") {
            std::string input = guiInputTxt("speed value (m/s)");
            try {
                player_speed = std::stof(input);
            } catch (std::exception &e) {
                printf("[ERROR] %s\n", e.what());
                notify(TextFormat("[ERROR] %s", e.what()));
            }
        }

        else {
            notify("Invalid command.");
        }
    }
}

c4v getC4v(Vector3i pos, int faceN) {
    return (smoothLighting ? world.genC4v(pos, faceN) : (c4v){1, 1, 1, 1});
}

void draw_block(Vector3i pos) {
    if (world.is_empty(pos)) {
        return;
    }

    static const float b_top = 1.0f, // top
        b1 = 0.9f,                   // left, right
        b2 = 0.8f,                   // front, back
        b_bottom = 0.7f;             // bottom

    static int lastT = -1, t = 0;
    lastT = -1;

    Faces faces = {
        (camera.position.x < pos.x && world.is_empty(pos + Vector3i{-1, 0, 0})),
        (camera.position.x > pos.x && world.is_empty(pos + Vector3i{+1, 0, 0})),
        (camera.position.y < pos.y && world.is_empty(pos + Vector3i{0, -1, 0})),
        (camera.position.y > pos.y && world.is_empty(pos + Vector3i{0, +1, 0})),
        (camera.position.z < pos.z && world.is_empty(pos + Vector3i{0, 0, -1})),
        (camera.position.z > pos.z && world.is_empty(pos + Vector3i{0, 0, +1})),
    };

    if (!faces.back && !faces.front && !faces.left && !faces.right &&
        !faces.top && !faces.bottom) {
        return;
    }

    uint8_t type_idx = (uint8_t)world.get_at(pos);
    auto sides = blockData[type_idx].sides;

    // Top Face
    if (faces.top) {
        t = block_textures[sides[0]].id;
        if (lastT != t) {
            rlSetTexture(t);
            lastT = t;
        }
        c4v vc = getC4v(pos, face_top);

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

        c4v vc = getC4v(pos, face_right);

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

        c4v vc = getC4v(pos, face_left);

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

        c4v vc = getC4v(pos, face_back);

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

        c4v vc = getC4v(pos, face_front);

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
        c4v vc = getC4v(pos, face_bottom);
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

void draw3D() {
    BeginMode3D(camera);

    int oldSeed = rand();

    int cc = (int)round(camera.position.y / CHUNK_SIZE);

    bool lookUp = (rot_y < 90.0f);

    // minimum
    int k1 = lookUp ? std::max(cc - 1, 0) : std::max(cc - render_dist, 0);
    // maximum
    int k2 = lookUp ? std::min(cc + render_dist, CHUNK_COUNT - 1)
                    : std::min(cc, CHUNK_COUNT - 1);

    // rlCheckRenderBatchLimit((blockCount + fallingBlocks.size())*3);
    rlPushMatrix();
    rlBegin(RL_QUADS);

    for (FallingBlock& fb : world.falling_blocks) {
        if (abs(fb.pos.y - camera.position.y) > render_dist * CHUNK_SIZE) {
            continue;
        }

        Faces f{
            camera.position.x<fb.pos.x, camera.position.x> fb.pos.x,
            camera.position.y<fb.pos.y, camera.position.y> fb.pos.y,
            camera.position.z<fb.pos.z, camera.position.z> fb.pos.z,
        };

        drawCubeTextureFaces(fb.pos, f, fb.type);
    }

    for (int chunk = k1; chunk < k2 + 1; chunk++) {
        for (int i = 0; i < CHUNK_SIZE; i++) {
            int y = chunk * CHUNK_SIZE + i;
            for (int x = 0; x < CHUNK_SIZE; x++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    draw_block({x, y, z});
                }
            }
        }
    }

    rlEnd();
    rlPopMatrix();

    if (world.on_map(looking_at)) {
        Color c = BLACK; // ColorFromHSV((float)GetTime()*20.0f, 1.0f, 1.0f);
        // DrawCube(lookAt, 1,1,1, c);
        float las = 1.05f;
        DrawCubeWiresV(looking_at.to_raylib(), {las, las, las}, c);
    }

    SetRandomSeed(oldSeed);

    EndMode3D();
}

void draw2D() {
#define ON_OFF(b) (b ? "ON" : "OFF")

    std::vector<std::string> infos{
        TextFormat("Version: %s", VERSION),
        TextFormat("xyz: %.1f %.1f %.1f", camera.position.x, camera.position.y,
                   camera.position.z),
        TextFormat("looking at: %s",
                   (looking_at.x == -1)
                       ? "----"
                       : TextFormat("%i, %i, %i (%i)", (int)looking_at.x,
                                    (int)looking_at.y, (int)looking_at.z,
                                    world.get_at(looking_at))),
        blockData[(std::size_t)cur_type].name,
        TextFormat("Y velocity: %.2f m/s (%.2f km/h)", vel_y * 20.0f,
                   vel_y * 20.0f * 3.6f),
        TextFormat("Rot (DEG): %.2f, %.2f", rot_x, rot_y),
        TextFormat("Fast place/break: %s", ON_OFF(fast_place)),
        TextFormat("Smooth lighting: %s", ON_OFF(smoothLighting)),
        TextFormat("flying: %s", ON_OFF(flying)),
        TextFormat("falling block count: %i", world.falling_blocks.size()),
    };

    float fontSize = (float)GetScreenHeight() / 40;
    for (size_t i = 0; i < infos.size(); i++) {
        // printf("%i: %s\n", i, texts[i].c_str());
        DrawTextEx(font, infos[i].c_str(),
                   {10, (float)(10 + i * (fontSize * 1.2f))}, fontSize, 2,
                   WHITE);
    }

    for (size_t i = 0; i < notifications.size(); i++) {
        int i2 = notifications.size() - 1 - i;
        unsigned char a = (i == 0)
            ? (unsigned char)(int)(std::clamp(notification_cooldown, 0.0f,
                                              1.0f) *
                                   255.0f)
            : 255;

        DrawTextC(font, notifications[i].c_str(),
                  {(float)GetScreenWidth() * 0.5f, 60 + (float)i2 * 30},
                  fontSize, (Color){255, 255, 255, a});
    }

    // draw selected block
    const Texture2D &tex = block_textures[blockData[(int)cur_type].sides[1]];

    float selected_block_size = (float)GetScreenHeight() * 0.1f;
    DrawRectangleRec({15, (float)GetScreenHeight() - (selected_block_size + 25),
                      (float)selected_block_size + 10,
                      (float)selected_block_size + 10},
                     BLACK);
    DrawTexturePro(tex, {0.0f, 0.0f, (float)tex.width, (float)tex.height},
                   {20, (float)GetScreenHeight() - selected_block_size - 20,
                    selected_block_size, selected_block_size},
                   {0, 0}, 0, WHITE);

    // draw crosshair
    drawCrosshair(WHITE);

    DrawFPS(0, 0);

    float keystroke_size = GetScreenWidth() * 0.03f;
    drawKeystrokes({GetScreenWidth() - keystroke_size * 3.3f, 0},
                   keystroke_size);
}

void draw() {
    BeginDrawing();
    ClearBackground(BACKGROUND_COLOR);

    draw3D();
    draw2D();

    EndDrawing();
}

void init() {
    InitWindow(1280, 720, "Game");

    SetExitKey(KEY_F4);
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    // SetWindowState(FLAG_VSYNC_HINT);

    DisableCursor();

    font = LoadFont(fontFName);
    init_textures("minecraft");

    InitAudioDevice();
    for (int i = 0; i < N_SOUNDS; i++) {
        sounds[i] = LoadSound(TextFormat("sound/%s.wav", soundNames[i]));
    }

    camera.position = (Vector3){CHUNK_SIZE / 2.0f, 20.0f, CHUNK_SIZE / 2.0f};
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = DEF_FOV;
    camera.projection = CAMERA_PERSPECTIVE;

    // load_world("pagoda", pCube);
    world.load("ch64", player_cube);
}

void deInit() {
    UnloadFont(font);

    for (int i = 0; i < TEXTURE_COUNT; i++) {
        UnloadTexture(block_textures[i]);
    }

    for (int i = 0; i < N_SOUNDS; i++) {
        UnloadSound(sounds[i]);
    }

    CloseWindow();
    // cLog(1, "closed without any errors");

    exit(0);
}

int main() {
    init();

    while (!WindowShouldClose()) {

        dt = GetFrameTime();
        draw();

        if (dt < 0.1f) {
            update();
        }
    }

    deInit();
}

std::string guiInputTxt(std::string displayText) {
    int key;
    std::string txt;

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(BACKGROUND_COLOR);

        draw3D();
        draw2D();

        DrawTextC(
            font, displayText.c_str(),
            {(float)GetScreenWidth() / 2, (float)GetScreenHeight() / 2 - 40},
            GetScreenHeight() / 10.0f, WHITE);
        DrawTextC(
            font, txt.c_str(),
            {(float)GetScreenWidth() / 2, (float)GetScreenHeight() / 2 + 40},
            GetScreenHeight() / 10.0f, WHITE);

        EndDrawing();

        key = GetCharPressed();

        // input text
        while (key > 0) {
            // 25, 125
            if (25 <= key && key <= 125) {
                txt.push_back((char)key);
            }
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE) && txt.length() > 0) {
            txt.resize(txt.size() - sizeof(char));
        }

        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }

        if (IsKeyReleased(KEY_ENTER)) {
            return txt;
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            return "";
        }
    }
    return txt;
}

void pause() {
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BACKGROUND_COLOR);
        draw3D();
        DrawRectangle(100, 100, 100, 100, RED);

        DrawTextC(
            font,
            "Paused.",
            {(float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() * 0.5f},
            GetScreenHeight() / 40.0f, WHITE);

        EndDrawing();

        if (IsKeyPressed(KEY_ESCAPE)) {
            return;
        }
    }
}

void blockSelectionMenu() {
    EnableCursor();

    int cols = 9, rows = (int)ceilf((float)BLOCK_TYPE_COUNT / (float)cols);
    int sb = GetScreenWidth() / 100;

    float scroll = 0;

    while (!WindowShouldClose()) {
        scroll += GetMouseWheelMove() * 50.0f;
        scroll = std::clamp(scroll, 0.0f, (float)GetScreenHeight() - 100);
        BeginDrawing();
        ClearBackground(BACKGROUND_COLOR);

        draw3D();

        DrawCircle(GetScreenWidth() / 2, GetScreenHeight() / 2 + 20, 10,
                   YELLOW);

        float block_type_size = (float)(GetScreenHeight() / 13.5f);

        uint8_t type_idx = 0;
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < cols; col++) {
                type_idx++;
                if (type_idx >= BLOCK_TYPE_COUNT) {
                    break;
                }

                const Texture2D &tex = block_textures[blockData[type_idx].sides[1]];

                Vector2 center = {
                    (float)GetScreenWidth() / 2 +
                        (col - cols / 2.0f) * (block_type_size + 10 + sb),
                    (float)(100 + row * (block_type_size + 10 + sb) + scroll)};

                DrawCircle(center.x, center.y, 10, RED);

                float outline_width = 5.0f;
                Rectangle rec = {
                    center.x - block_type_size * 0.5f + outline_width,
                    center.y - block_type_size * 0.5f + outline_width,
                    (float)block_type_size + 2 * outline_width,
                    (float)block_type_size + 2 * outline_width};

                DrawRectangleRec(
                    rec,
                    //{(float)CX-(cTypeShowSize/2+5),(float)H-(cTypeShowSize+25),(float)cTypeShowSize+10,(float)cTypeShowSize+10},
                    cur_type == (BlockType)type_idx ? WHITE : BLACK);

                DrawTexturePro(
                    tex, {0.0f, 0.0f, (float)tex.width, (float)tex.height},
                    {center.x - block_type_size * 0.5f,
                     center.y - block_type_size * 0.5f, block_type_size,
                     block_type_size},
                    {0.0f, 0.0f}, 0.0f, WHITE);

                if (CheckCollisionPointRec(GetMousePosition(), rec)) {
                    DrawTextC(font, blockData[type_idx].name,
                              {center.x, center.y - block_type_size * 0.8f},
                              GetScreenHeight() / 40.0f, WHITE);
                    if (CheckCollisionPointRec(GetMousePosition(), rec) &&
                        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        cur_type = (BlockType)type_idx;
                    }
                }
            }
        }

        EndDrawing();

        if (IsKeyPressed(KEY_E)) {
            break;
        }
    }

    DisableCursor();
}
