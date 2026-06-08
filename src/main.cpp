
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

static int cur_type = 0, render_dist = 2;

static bool able_to_jump = false;
static bool flying = false;

static bool fast_place = 0;
static bool falling_enabled = true;
static bool smoothLighting = true;

static Camera3D camera;

// static const int cTypeShowSize = (float)(H / 13.5f), fontSize = H / 40;

static Vector3 looking_at = {0, 0, 0};

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
                if (!is_empty(x, y + a, z)) {
                    placed = false;
                }
            }

            if (is_empty(x, y, z)) {
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
    int x1, y1, z1, x2, y2, z2;
    char type;

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

        x1 = std::stoi(inputPos1[0]);
        y1 = std::stoi(inputPos1[1]);
        z1 = std::stoi(inputPos1[2]);

        x2 = std::stoi(inputPos2[0]);
        y2 = std::stoi(inputPos2[1]);
        z2 = std::stoi(inputPos2[2]);

        type = std::stoi(guiInputTxt("type"));

        if (type < -1 || type > BLOCK_TYPE_COUNT - 1) {
            notify("Type out of range.");
            return;
        }
    } catch (const std::invalid_argument &e) {
        notify(e.what());
        return;
    } catch (const std::out_of_range &e) {
        notify(e.what());
        return;
    }

    if (!on_map(x1, y1, z1) || !on_map(x2, y2, z2)) {
        notify("Not on map.");
        return;
    }

    Vector3 p1{(float)std::min(x1, x2), (float)std::min(y1, y2),
               (float)std::min(z1, z2)};
    Vector3 p2{(float)std::max(x1, x2), (float)std::max(y1, y2),
               (float)std::max(z1, z2)};

    int c, rY;

    for (int x = (int)p1.x; x < (int)p2.x + 1; x++) {
        for (int y = (int)p1.y; y < (int)p2.y + 1; y++) {
            c = (int)(y / CHUNK_SIZE);
            rY = (y % CHUNK_SIZE);
            for (int z = (int)p1.z; z < (int)p2.z + 1; z++) {
                blocks[c][x][rY][z] = type;
            }
        }
    }
}

void update_block() {

}

void tick() {
    if (!flying) {
        vel_y -= 0.08f;
        vel_y *= 0.98f;
    }

    if (falling_enabled) {
        for (FallingBlock &fb : falling_blocks) {
            fb.vel.y -= 0.08f;
            fb.vel.y *= 0.98f;
        }
    }

    for (size_t j = 0; falling_enabled && j < falling_blocks.size(); j++) {
        FallingBlock &fb = falling_blocks[j];
        fb.vel.y -= 0.08f;
        fb.vel.y *= 0.98f;
        if (fb.pos.y < -200) {
            falling_blocks.erase(falling_blocks.begin() + j);
            j--;
        }
        // fb.pos += fb.vel; -- done elsewhere
    }

    int player_chunk = player_cube.pos.y / CHUNK_SIZE;
    for (int c = std::clamp(player_chunk - 1, 0, CHUNK_COUNT - 1);
         c < std::clamp(player_chunk + 1, 0, CHUNK_COUNT - 1) + 1; c++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    int8_t type = blocks[c][x][y][z];

                    if (type == bt_air) {
                        continue;
                    }

                    switch (type) {

                    case bt_grass: {
                        if (get_at(x, y + c * CHUNK_SIZE + 1, z) != bt_air) {
                            blocks[c][x][y][z] = bt_dirt;
                        }

                        break;
                    }

                    case bt_dirt: {
                        if (bt_air != get_at(x, y + 1 + c * CHUNK_SIZE, z)) {
                            break;
                        }

                        int grass_neighbors = 0;

                        for (int x2 = -1; x2 <= 1; x2++) {
                            for (int y2 = -1; y2 <= 1; y2++) {
                                for (int z2 = -1; z2 <= 1; z2++) {
                                    if (0 != x2 + z2 &&
                                        on_map(x + x2, y + y2 + c * CHUNK_SIZE,
                                               z + z2) &&
                                        bt_grass ==
                                            get_at(x + x2,
                                                   y + y2 + c * CHUNK_SIZE,
                                                   z + z2)) {
                                        grass_neighbors++;
                                    }
                                }
                            }
                        }

                        if (grass_neighbors > 0 && rand() % 10 == 0) {
                            set_at(x, y + c * CHUNK_SIZE, z, bt_grass);
                        }

                        break;
                    }

                    case bt_sand: {
                        if (get_at(x, y + c * CHUNK_SIZE - 1, z) == bt_air) {

                            falling_blocks.push_back((FallingBlock){
                                {0.0f, 0.0f, 0.0f},
                                (Vector3){(float)x, (float)(y + c * CHUNK_SIZE),
                                          (float)z},
                                type});

                            set_at(x, y + c * CHUNK_SIZE, z, bt_air);
                        }
                        break;
                    }

                    default: {
                        break;
                    }
                    }
                }
            }
        }
    }
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

    cur_type += GetMouseWheelMove();
    cur_type = std::min(std::max(cur_type, 0), BLOCK_TYPE_COUNT - 1);

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

    for (size_t j = 0; falling_enabled && j < falling_blocks.size(); j++) {
        FallingBlock &fb = falling_blocks[j];
        Vector3 delta = fb.vel * dt * 20.0f;
        if (get_at(fb.pos + delta) == bt_air) {
            fb.pos += delta;
        } else {
            if (fb.type == bt_wool_red) {
                set_at(fb.pos + delta, bt_air);
            } else {
                set_at(fb.pos, fb.type);
            }
            falling_blocks.erase(falling_blocks.begin() + j);
            j--;
        }
    }

    Vector3 player_pos_delta;

    player_pos_delta.x =
        (dCos(rot_x) * (forward - back) + -dSin(rot_x) * (right - left));
    player_pos_delta.z =
        (dSin(rot_x) * (forward - back) + dCos(rot_x) * (right - left));
    // * playerSpeed * dt

    // float vec_length = Vector3Length(player_pos_delta);
    // if (vec_length > 0) {
    //   player_pos_delta /= vec_length;
    // }

    player_pos_delta *= player_speed * dt;

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
                if (flying || !on_map(x, y, z) || is_empty(x, y, z)) {
                    continue;
                }

                Cube cube = blockCube(x, y, z);

                if (player_cube.movedCopy(player_pos_delta.x, 0, 0)
                        .collide(cube)) {
                    player_pos_delta.x = 0;
                }

                if (player_cube.movedCopy(0, player_pos_delta.y, 0)
                        .collide(cube)) {
                    player_pos_delta.y = 0;
                    vel_y = 0;

                    if (player_cube.getTLF().y + PLAYER_HEIGHT > (float)y) {
                        able_to_jump = true;
                        if (get_at(x, y, z) == bt_slime) {
                            vel_y = 4;
                        }
                    }
                }

                if (player_cube.movedCopy(0, 0, player_pos_delta.z)
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

    camera.target.y = camera.position.y + dCos(rot_y);
    camera.target.x = camera.position.x + dSin(rot_y) * dCos(rot_x);
    camera.target.z = camera.position.z + dSin(rot_y) * dSin(rot_x);

    // Toggle fullscreen
    if (IsKeyPressed(KEY_F11)) {
        ToggleFullscreen();
    }

    // zoom
    cur_fov = IsKeyDown(KEY_C) ? (DEF_FOV * 0.3f) : DEF_FOV;
    camera.fovy += dt * 20 * (cur_fov - camera.fovy);

    Vector3 placeAdd{0, 0, 0};
    for (float cRad = 0; cRad < reach + REACH_STEP; cRad += REACH_STEP) {
        Vector3 lookAtf{dSin(rot_y) * dCos(rot_x) * cRad + camera.position.x,
                        dCos(rot_y) * cRad + camera.position.y,
                        dSin(rot_y) * dSin(rot_x) * cRad + camera.position.z};
        looking_at =
            (Vector3){roundf(lookAtf.x), roundf(lookAtf.y), roundf(lookAtf.z)};

        // block found
        if (!is_empty(looking_at)) {

            float dx = lookAtf.x - looking_at.x;
            float dy = lookAtf.y - looking_at.y;
            float dz = lookAtf.z - looking_at.z;

            int x = looking_at.x;
            int y = looking_at.y;
            int z = looking_at.z;

            float add = 0.5f - REACH_STEP;

            if (dx < -add && is_empty(x - 1, y, z)) {
                placeAdd = (Vector3){-1, 0, 0};
            } else if (dx > add && is_empty(x + 1, y, z)) {
                placeAdd = (Vector3){1, 0, 0};
            } else if (dy < -add && is_empty(x, y - 1, z)) {
                placeAdd = (Vector3){0, -1, 0};
            } else if (dy > add && is_empty(x, y + 1, z)) {
                placeAdd = (Vector3){0, 1, 0};
            } else if (dz < -add && is_empty(x, y, z - 1)) {
                placeAdd = (Vector3){0, 0, -1};
            } else if (dz > add && is_empty(x, y, z + 1)) {
                placeAdd = (Vector3){0, 0, 1};
            }

            break;
        }

        if (cRad >= reach) {
            looking_at.x = -1;
            placeAdd = (Vector3){0, 0, 0};
        }
    }

    if (IsKeyPressed(KEY_P)) {
        fast_place = !fast_place;
    }

    if (IsKeyPressed(KEY_T)) {
        player_cube.pos = tp_point;
        notify(TextFormat("Teleported to: %.1f, %.1f, %.1f", tp_point.x,
                          tp_point.y, tp_point.z));
    }

    Vector3 placeAt = Vector3Add(looking_at, placeAdd);

    if ((IsKeyDown(KEY_X) || IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) &&
        on_map(looking_at) && !is_empty(looking_at)) {
        cur_type = get_at(looking_at);
    }

    bool action_break = (
        (fast_place ? IsKeyDown : IsKeyPressed)(KEY_U) ||
        (fast_place ? IsMouseButtonDown : IsMouseButtonPressed)(MOUSE_LEFT_BUTTON)
    );
    if (action_break &&
        !is_empty((int)looking_at.x, (int)looking_at.y, (int)looking_at.z) &&
        on_map(looking_at) && on_map(looking_at)) {
        set_at(looking_at, bt_air);
        PlaySound(sounds[SOUND_BREAK]);
    }

    bool action_place = (
        (fast_place ? IsKeyDown : IsKeyPressed)(KEY_O) ||
        (fast_place ? IsMouseButtonDown : IsMouseButtonPressed)(MOUSE_RIGHT_BUTTON)
    );
    if (action_place && (flying || !player_cube.collide(blockCube(placeAt)))) {
        if (on_map(placeAt) && is_empty(placeAt)) {
            set_at(placeAt, cur_type);
            // fallingBlocks.push_back({0, {placeAt.x, placeAt.y+0.5f,
            // placeAt.z},
            set_at(placeAt, cur_type);
            PlaySound(sounds[SOUND_PLACE]);
        } else if (looking_at.x == -1) {
            Vector3 vel{dSin(rot_y) * dCos(rot_x), dCos(rot_y),
                        dSin(rot_y) * dSin(rot_x)};
            Vector3 pos = player_cube.pos + player_cube.size / 2 + vel * 2;
            vel *= 2;

            falling_blocks.push_back((FallingBlock){
                vel,
                pos,
                cur_type,
            });
        }
    }

    if (on_map(placeAt) && IsKeyPressed(KEY_I)) {
        gen_tree(placeAt.x, placeAt.y, placeAt.z);
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        pause();
    }

    if (IsKeyPressed(KEY_G)) {
        falling_enabled = !falling_enabled;
    }

    if (IsKeyPressed(KEY_E)) {
        blockSelectionMenu();
    }

    if (IsKeyDown(KEY_Q) && looking_at.x != -1) {
        blocks[chunk_of(looking_at.y)][(int)looking_at.x]
              [(int)looking_at.y % CHUNK_SIZE][(int)looking_at.z] = cur_type;
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
        if (fName != "" && save_world(fName, player_cube) == 0) {
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
        if (fName != "" && load_world(fName, player_cube) == 0) {
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
        clear_world();
    }

    // change texture pack
    if (ctrl && IsKeyPressed(KEY_Z)) {
        std::string name = guiInputTxt("texture pack name");
        initTextures(name.c_str());
    }

    if (ctrl && IsKeyPressed(KEY_B)) {
        std::string command = guiInputTxt("command:");

        if (command == "fill") {
            command_fill();
        }

        else if (command == "replace") {
            char t1 = 0, t2 = 0;

            try {
                t1 = std::stoi(guiInputTxt("from"));
                t2 = std::stoi(guiInputTxt("to"));
            } catch (std::exception &e) {
                printf("[ERROR] %s", e.what());
                notify(TextFormat("[ERROR] %s", e.what()));
            }

            for (int c = 0; c < CHUNK_COUNT; c++) {
                for (int x = 0; x < CHUNK_SIZE; x++) {
                    for (int y = 0; y < CHUNK_SIZE; y++) {
                        for (int z = 0; z < CHUNK_SIZE; z++) {
                            if (blocks[c][x][y][z] == t1) {
                                blocks[c][x][y][z] = t2;
                            }
                        }
                    }
                }
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

        else if (command == "rand_replace") {
            std::string input = guiInputTxt("block to replace");
            try {
                int rbid = std::stof(input);

                for (int c = 0; c < CHUNK_COUNT; c++) {
                    for (int y = 0; y < CHUNK_SIZE; y++) {
                        for (int x = 0; x < CHUNK_SIZE; x++) {
                            for (int z = 0; z < CHUNK_SIZE; z++) {

                                if (blocks[c][x][y][z] == rbid) {
                                    blocks[c][x][y][z] =
                                        rand() % BLOCK_TYPE_COUNT;
                                }
                            }
                        }
                    }
                }

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

Faces getFaces(int x, int y, int z) {
    const float cx = camera.position.x, cy = camera.position.y,
                cz = camera.position.z;
    const bool lt = false;

    return {
        (cx < x &&
         (is_empty(x - 1, y, z) ||
          (lt && blockData[get_at(x - 1, y, z)].translucent))),
        (cx > x &&
         (is_empty(x + 1, y, z) ||
          (lt && blockData[get_at(x + 1, y, z)].translucent))),
        (cy < y &&
         (is_empty(x, y - 1, z) ||
          (y > 0 && lt && blockData[get_at(x, y - 1, z)].translucent))),
        (cy > y &&
         (is_empty(x, y + 1, z) ||
          (y < CHUNK_SIZE * CHUNK_COUNT && lt &&
           blockData[get_at(x, y + 1, z)].translucent))),
        (cz < z &&
         (is_empty(x, y, z - 1) ||
          (z > 0 && lt && blockData[get_at(x, y, z - 1)].translucent))),
        (cz > z &&
         (is_empty(x, y, z + 1) ||
          (z < CHUNK_SIZE && lt &&
           blockData[get_at(x, y, z + 1)].translucent))),
    };
}

c4v getC4v(int x, int y, int z, int faceN) {
    return (smoothLighting ? genC4v(x, y, z, faceN) : (c4v){1, 1, 1, 1});
}

void draw_block(int x, int y, int z) {
    static const float b_top = 1.0f, // top
        b1 = 0.9f,                   // left, right
        b2 = 0.8f,                   // front, back
        b_bottom = 0.7f;             // bottom

    static int lastT, t;
    lastT = -1;

    if (get_at(x, y, z) == bt_air) {
        return;
    }

    Faces faces = getFaces(x, y, z);
    if (!faces.back && !faces.front && !faces.left && !faces.right &&
        !faces.top && !faces.bottom) {
        return;
    }

    int type = get_at(x, y, z);

    if (1 && type == bt_stone && is_empty(x, y * CHUNK_SIZE + 1, z)) {
        DrawBillboard(camera, textures[blockData[bt_wool_red].sides[1]],
                      {(float)x, (float)(y + 1), (float)z}, 1, WHITE);
    }

    // === DRAW FACES ===

    // Top Face
    if (faces.top) {
        t = textures[blockData[type].sides[0]].id;
        if (lastT != t) {
            rlSetTexture(t);
            lastT = t;
        }
        c4v vc = getC4v(x, y, z, face_top);

        rlNormal3f(0.0f, 1.0f, 0.0f); // Normal Pointing Up

        color4ubGF(vc.tl * b_top);
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(x - 0.5f, y + 0.5f, z - 0.5f); // Top Left of the Quad

        color4ubGF(vc.br * b_top);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(x - 0.5f, y + 0.5f,
                   z + 0.5f); // Bottom Left Of The Quad

        color4ubGF(vc.tr * b_top);
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(x + 0.5f, y + 0.5f,
                   z + 0.5f); // Bottom Right Of The Quad

        color4ubGF(vc.bl * b_top);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(x + 0.5f, y + 0.5f, z - 0.5f); // Top Right Of The Quad
    }

    // Right face
    t = textures[blockData[type].sides[1]].id;
    if (t != lastT &&
        (faces.right || faces.left || faces.back || faces.front)) {
        rlSetTexture(t);
        lastT = t;
    }

    if (faces.right) {
        rlNormal3f(1.0f, 0.0f, 0.0f); // Normal Pointing Right
        // color4ubG(c*b1, 255, 0);

        c4v vc = getC4v(x, y, z, face_right);

        color4ubGF(vc.br * b1);
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(x + 0.5f, y - 0.5f,
                   z - 0.5f); // Bottom Right Of The Texture and Quad

        color4ubGF(vc.tl * b1);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(x + 0.5f, y + 0.5f,
                   z - 0.5f); // Top Right Of The Texture and Quad

        color4ubGF(vc.bl * b1);
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(x + 0.5f, y + 0.5f,
                   z + 0.5f); // Top Left Of The Texture and Quad

        color4ubGF(vc.tr * b1);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(x + 0.5f, y - 0.5f,
                   z + 0.5f); // Bottom Left Of The Texture and Quad
    }

    // Left Face
    // if(blockTextureI[type][0] != blockTextureI[type][1])
    // rlSetTexture(textures[blockTextureI[type][0]].id);
    if (faces.left) {
        rlNormal3f(-1.0f, 0.0f, 0.0f); // Normal Pointing Left
        // color4ubG(c*b1, 255, 0);

        c4v vc = getC4v(x, y, z, face_left);

        color4ubGF(vc.br * b1);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(x - 0.5f, y - 0.5f,
                   z - 0.5f); // Bottom Left Of The Texture and Quad

        color4ubGF(vc.tr * b1);
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(x - 0.5f, y - 0.5f,
                   z + 0.5f); // Bottom Right Of The Texture and Quad

        color4ubGF(vc.bl * b1);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(x - 0.5f, y + 0.5f,
                   z + 0.5f); // Top Right Of The Texture and Quad

        color4ubGF(vc.tl * b1);
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(x - 0.5f, y + 0.5f,
                   z - 0.5f); // Top Left Of The Texture and Quad
    }

    // Front Face

    if (faces.back) {
        // color4ubG(c*b2, 255, 0);
        rlNormal3f(0.0f, 0.0f, 1.0f); // Normal Pointing Towards Viewer

        c4v vc = getC4v(x, y, z, face_back);

        color4ubGF(vc.br * b2);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(x - 0.5f, y - 0.5f,
                   z + 0.5f); // Bottom Left Of The Texture and Quad

        color4ubGF(vc.tr * b2);
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(x + 0.5f, y - 0.5f,
                   z + 0.5f); // Bottom Right Of The Texture and Quad

        color4ubGF(vc.bl * b2);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(x + 0.5f, y + 0.5f,
                   z + 0.5f); // Top Right Of The Texture and Quad

        color4ubGF(vc.tl * b2);
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(x - 0.5f, y + 0.5f,
                   z + 0.5f); // Top Left Of The Texture and Quad
    }

    // Back Face
    if (faces.front) {
        // color4ubG(c*b2, 255, 0);
        // rlNormal3f(0.0f, 0.0f, - 1.0f);                  // Normal
        // Pointing Away From Viewer

        c4v vc = getC4v(x, y, z, face_front);

        color4ubGF(vc.br * b2);
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(x - 0.5f, y - 0.5f,
                   z - 0.5f); // Bottom Right Of The Texture and Quad

        color4ubGF(vc.tl * b2);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(x - 0.5f, y + 0.5f,
                   z - 0.5f); // Top Right Of The Texture and Quad

        color4ubGF(vc.bl * b2);
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(x + 0.5f, y + 0.5f,
                   z - 0.5f); // Top Left Of The Texture and Quad

        color4ubGF(vc.tr * b2);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(x + 0.5f, y - 0.5f,
                   z - 0.5f); // Bottom Left Of The Texture and Quad
    }

    // Bottom Face
    if (faces.bottom) {
        t = textures[blockData[type].sides[2]].id;
        if (lastT != t) {
            rlSetTexture(t);
            lastT = t;
        }

        // color4ubG(c*b3, 255, rgb);
        rlNormal3f(0.0f, -1.0f, 0.0f); // Normal Pointing Down

        c4v vc = getC4v(x, y, z, face_bottom);
        color4ubGF(vc.tl * b_bottom);
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(x - 0.5f, y - 0.5f,
                   z - 0.5f); // Top Right Of The Texture and Quad

        color4ubGF(vc.bl * b_bottom);
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(x + 0.5f, y - 0.5f,
                   z - 0.5f); // Top Left Of The Texture and Quad

        color4ubGF(vc.tr * b_bottom);
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(x + 0.5f, y - 0.5f,
                   z + 0.5f); // Bottom Left Of The Texture and Quad

        color4ubGF(vc.br * b_bottom);
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(x - 0.5f, y - 0.5f,
                   z + 0.5f); // Bottom Right Of The Texture and Quad
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

    for (size_t i = 0; i < falling_blocks.size(); i++) {
        FallingBlock &fb = falling_blocks[i];

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
                    draw_block(x, y, z);
                }
            }
        }
    }

    rlEnd();
    rlPopMatrix();

    if (on_map(looking_at)) {
        Color c = BLACK; // ColorFromHSV((float)GetTime()*20.0f, 1.0f, 1.0f);
        // DrawCube(lookAt, 1,1,1, c);
        float las = 1.05f;
        DrawCubeWiresV({looking_at.x, looking_at.y, looking_at.z},
                       {las, las, las}, c);
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
                                    get_at(looking_at))),
        blockData[cur_type].name,
        TextFormat("Y velocity: %.2f m/s (%.2f km/h)", vel_y * 20.0f,
                   vel_y * 20.0f * 3.6f),
        TextFormat("Rot (DEG): %.2f, %.2f", rot_x, rot_y),
        TextFormat("Fast place/break: %s", ON_OFF(fast_place)),
        TextFormat("Smooth lighting: %s", ON_OFF(smoothLighting)),
        TextFormat("flying: %s", ON_OFF(flying)),
        TextFormat("falling block count: %i", falling_blocks.size()),
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

        DrawTextC(notifications[i].c_str(),
                  {(float)GetScreenWidth() * 0.5f, 60 + (float)i2 * 30},
                  fontSize, (Color){255, 255, 255, a});
    }

    // draw selected block
    const Texture2D &tex = textures[blockData[cur_type].sides[1]];

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
    initTextures("mc16x");

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
    load_world("ch64", player_cube);
}

void deInit() {
    UnloadFont(font);

    for (int i = 0; i < TEXTURE_COUNT; i++) {
        UnloadTexture(textures[i]);
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
            displayText.c_str(),
            {(float)GetScreenWidth() / 2, (float)GetScreenHeight() / 2 - 40},
            GetScreenHeight() / 10.0f, WHITE);
        DrawTextC(
            txt.c_str(),
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

    bool run = true;

    int cols = 9, rows = (int)ceilf((float)BLOCK_TYPE_COUNT / (float)cols);
    int sb = GetScreenWidth() / 100;

    float scroll = 0;

    while (!WindowShouldClose() && run) {
        scroll += GetMouseWheelMove() * 50.0f;
        scroll = std::clamp(scroll, 0.0f, (float)GetScreenHeight() - 100);
        BeginDrawing();
        ClearBackground(BACKGROUND_COLOR);

        draw3D();

        DrawCircle(GetScreenWidth() / 2, GetScreenHeight() / 2 + 20, 10,
                   YELLOW);

        float block_type_size = (float)(GetScreenHeight() / 13.5f);

        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < cols; col++) {
                int type = row * cols + col;
                if (type > BLOCK_TYPE_COUNT - 1) {
                    break;
                }

                const Texture2D &tex = textures[blockData[type].sides[1]];

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
                    cur_type == type ? WHITE : BLACK);

                DrawTexturePro(
                    tex, {0.0f, 0.0f, (float)tex.width, (float)tex.height},
                    {center.x - block_type_size * 0.5f,
                     center.y - block_type_size * 0.5f, block_type_size,
                     block_type_size},
                    {0.0f, 0.0f}, 0.0f, WHITE);

                if (CheckCollisionPointRec(GetMousePosition(), rec)) {
                    DrawTextC(blockData[type].name,
                              {center.x, center.y - block_type_size * 0.8f},
                              GetScreenHeight() / 40.0f, WHITE);
                    if (CheckCollisionPointRec(GetMousePosition(), rec) &&
                        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        cur_type = type;
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
