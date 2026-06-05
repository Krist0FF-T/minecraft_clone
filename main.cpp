
#include <stdio.h>
#include <stdlib.h>

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#define GLSL_VERSION 330

// own
#include "blocktypes.hpp"
#include "draw.hpp"
#include "second.hpp"
#include "sound.hpp"
#include "third.hpp"
#include "world.hpp"

#define VERSION "0.0.6 (DEVMODE)"

static const float PLAYER_WIDTH = 0.6f, PLAYER_HEIGHT = 1.8f;
static Cube player_cube {{0, 0, 0}, {PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_WIDTH}};

// static const float gravity = 9.8f;
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

static int rad = 3, cur_type = 0, render_dist = 2;

static bool able_to_jump = false;
static bool fast_place = 0;
static bool running = true, falling_enabled = true,
            smoothLighting = true, flying = false;

static Camera3D camera;

// static const int cTypeShowSize = (float)(H / 13.5f), fontSize = H / 40;

static Vector3 looking_at = {0, maxChunks *chunkSize - 3, 0};

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
    {(float) GetScreenWidth() * 0.5f, (float) GetScreenHeight()/2 - chSize},
    {(float) GetScreenWidth() * 0.5f, (float) GetScreenHeight()/2 + chSize},
    chLineWidth,
    color
  );

  DrawLineEx(
    {(float)GetScreenWidth() * 0.5f - chSize, (float) GetScreenHeight()/2},
    {(float)GetScreenWidth() * 0.5f + chSize, (float) GetScreenHeight()/2},
    chLineWidth,
    color
  );
}

std::string guiInputTxt(std::string displayText);

// declaration of pause function
void pause();
void blockSelectionMenu();

bool blockCollidePlayer(int x, int y, int z, Vector3 p) {

  float add = PLAYER_WIDTH;
  return (p.x > (float)(x - add) && p.x < (float)(x + add) &&

          p.y > (float)(y - PLAYER_HEIGHT * 0.25f) &&
          p.y < (float)(y + PLAYER_HEIGHT * 1.75f) &&

          p.z > (float)(z - PLAYER_WIDTH / 2) && p.z < (float)(z + PLAYER_WIDTH / 2));
}

void placePlayer() {

  bool placed = false;

  while (!placed) {
    int x = rand() % 32;
    int z = rand() % 32;

    for (int y = maxChunks * chunkSize - 1; y > -1 && !placed; y--) {
      placed = true;
      for (int a = 1; a < 3 && placed; a++) {
        if (!is_empty(x, y + a, z)) {
          placed = false;
        }
      }

      if (is_empty(x, y, z))
        placed = false;

      if (placed) {
        player_cube.pos.x = x;
        player_cube.pos.y = y + PLAYER_HEIGHT;
        player_cube.pos.z = z;
      }
    }
  }
}

void genWorld() {
  std::string input = guiInputTxt("World generation seed");
  // bool err = false;

  int seed = rand();
  try {
    seed = std::stoi(input);
  } catch (const std::invalid_argument &e) {
    printf("%s\n", e.what());
  } catch (const std::out_of_range &e) {
    printf("%s\n", e.what());
  }

  genMap2(seed);
  printf("---- world generated!\n");

  falling_blocks.clear();
  placePlayer();
}

void command_fill() {
  int x1, y1, z1, x2, y2, z2;
  char type;

  try {
    std::vector<std::string> inputPos1 =
        splitStr(guiInputTxt("pos1: [x] [y] [z]"), ' ');
    if (inputPos1.size() != 3) {
      notify("correct format: [x] [y] [z]");
      return;
    }

    std::vector<std::string> inputPos2 =
        splitStr(guiInputTxt("pos2: [x] [y] [z]"), ' ');
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

    if (type < -1 || type > blockTypeCount - 1) {
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

  Vector3 p1{(float) min(x1, x2), (float) min(y1, y2), (float) min(z1, z2)};

  Vector3 p2{(float) max(x1, x2), (float) max(y1, y2), (float) max(z1, z2)};

  int c, rY;

  for (int x = (int) p1.x; x < (int) p2.x + 1; x++) {
  for (int y = (int) p1.y; y < (int) p2.y + 1; y++) {
    c = (int)(y / chunkSize);
    rY = (y % chunkSize);
  for (int z = (int) p1.z; z < (int) p2.z + 1; z++) {
    blocks[c][x][rY][z] = type;
  }}}
}

void tick() {
  if (!flying) {
    vel_y -= 0.08f;
    vel_y *= 0.98f;
  }

  // player chunk
  int pc = player_cube.pos.y / chunkSize, type;

  for (size_t j = 0; falling_enabled && j < falling_blocks.size(); j++) {
    FallingBlock& fb = falling_blocks[j];
    fb.vel.y -= 0.08f;
    fb.vel.y *= 0.98f;
    if (fb.pos.y < -200) {
      falling_blocks.erase(falling_blocks.begin() + j);
      j--;
    }
    // fb.pos += fb.vel; -- done elsewhere
  }

  for (
      int c = min(max(pc - 1, 0), maxChunks - 1);
      c < min(max(pc + 1, 0), maxChunks - 1) + 1;
      c++
  ) {
    for (int x = 0; x < chunkSize; x++) {
    for (int y = 0; y < chunkSize; y++) {
    for (int z = 0; z < chunkSize; z++) {
      type = blocks[c][x][y][z];
      if (type == bt_air) {
        continue;
      }

      switch (type) {

      case bt_grass: {
        if (get_at(x, y + c * chunkSize + 1, z) != bt_air)
          blocks[c][x][y][z] = bt_dirt;

        break;
      }

      case bt_dirt: {
        if (bt_air != get_at(x, y + 1 + c * chunkSize, z)) {
          break;
        }

        int grass_neighbors = 0;

        for (int x2 = -1; x2 <= 1; x2++) {
        for (int y2 = -1; y2 <= 1; y2++) {
        for (int z2 = -1; z2 <= 1; z2++) {
          if (0 != x2 + z2
              && on_map(x + x2, y + y2 + c * chunkSize, z + z2)
              && bt_grass == get_at(x + x2, y + y2 + c * chunkSize, z + z2)
          ) {
            grass_neighbors++;
          }
        }}}

        if (grass_neighbors > 0 && chanceP(grass_neighbors)) {
        // if (grass_neighbors > 0) {
          set_at(x, y + c * chunkSize, z, bt_grass);
        }

        break;
      }

      case bt_sand: {
        if (c == 0 && y == 0)
          break;
        if (get_at(x, y + c * chunkSize - 1, z) == bt_air) {

          falling_blocks.push_back((FallingBlock) {
            {0.0f, 0.0f, 0.0f},
            (Vector3) {
              (float) x,
              (float) (y + c * chunkSize),
              (float) z
            },
            type
          });

          set_at(x, y + c * chunkSize, z, bt_air);
        }
        break;
      }

      default: {
        break;
      }
      }
    }}}
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

  cur_type += sign(GetMouseWheelMove());
  cur_type = min(max(cur_type, 0), blockTypeCount - 1);

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
    FallingBlock& fb = falling_blocks[j];
    Vector3 delta = fb.vel * dt * 20.0f;
    if (get_at(fb.pos + delta) == bt_air) {
      fb.pos += delta;
    }
    else {
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

  player_pos_delta.x = (
    dCos(rot_x) * (forward - back) +
    -dSin(rot_x) * (right - left)
  );
  player_pos_delta.z = (
    dSin(rot_x) * (forward - back) +
    dCos(rot_x) * (right - left)
  );
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

  for (
    int x = (int)fFloor(player_cube.getTLF().x) - 1;
    // NOTE: might be a bug here - testW was unused before and it worked
    x < (int)fCeil(player_cube.getBRB().x) + 2 + testW + 1;
    x++
  ) {
  for (
    int y = (int)fFloor(player_cube.getTLF().y) - 1;
    y < (int)fCeil(player_cube.getBRB().y) + 2 + testH + 1;
    y++
  ) {
  for (
    int z = (int)fFloor(player_cube.getTLF().z) - 1;
    z < (int)fCeil(player_cube.getBRB().z) + 2 + testL + 1;
    z++
  ) {
    if (flying || !on_map(x, y, z) || is_empty(x, y, z)) {
      continue;
    }

    Cube cube = blockCube(x, y, z);

    if (player_cube.movedCopy(player_pos_delta.x, 0, 0).collide(cube)) {
      player_pos_delta.x = 0;
    }

    if (player_cube.movedCopy(0, player_pos_delta.y, 0).collide(cube)) {
      player_pos_delta.y = 0;
      vel_y = 0;

      if (player_cube.getTLF().y + PLAYER_HEIGHT > (float)y) {
        able_to_jump = true;
        if (get_at(x, y, z) == bt_slime) {
          vel_y = 4;
        }
      }
    }

    if (player_cube.movedCopy(0, 0, player_pos_delta.z).collide(cube)) {
      player_pos_delta.z = 0;
    }

  }}}

  player_cube.pos += player_pos_delta;
  // pCube.pos.x += dx;
  // pCube.pos.y += dy;
  // pCube.pos.z += dz;

  // if(!flying && pCube.pos.y < -10) pCube.pos.y = chunkSize*maxChunks+2;

  // pCube.pos = {camera.position.x, camera.position.y+1, camera.position.z};
  camera.position = {player_cube.pos.x, player_cube.pos.y + PLAYER_EYE, player_cube.pos.z};

  // view up & down

  const float rot_speed = IsKeyDown(KEY_LEFT_ALT) ? 8.0f : 1.25f;

  // rotX += GetMouseDelta().x * 0.1f;
  rot_x += (IsKeyDown(KEY_L) - IsKeyDown(KEY_H))*camera.fovy * rot_speed * dt;
  if (rot_x > 360) rot_x -= 360;
  if (rot_x < 0) rot_x += 360;

  // rotY += (float) GetMouseDelta().y / 10.0f;
  rot_y += (IsKeyDown(KEY_J) - IsKeyDown(KEY_K))*camera.fovy * rot_speed * dt;
  rot_y = min(max(rot_y, 0.1f), 179.9f);

  camera.target.y = dCos(rot_y) * rad + camera.position.y;
  camera.target.x = dSin(rot_y) * dCos(rot_x) * rad + camera.position.x;
  camera.target.z = dSin(rot_y) * dSin(rot_x) * rad + camera.position.z;

  // Toggle fullscreen
  if (IsKeyPressed(KEY_F11)) {
    ToggleFullscreen();
  }

  // SetMousePosition(CX, CY); -- doesn't really work on wayland...

  // zoom
  cur_fov = IsKeyDown(KEY_C) ? (DEF_FOV * 0.3f) : DEF_FOV;
  camera.fovy += dt * 20 * (cur_fov - camera.fovy);
  // camera.fovy = curFov;

  // if (IsKeyPressed(KEY_R)) {
  //   camera.projection = 1 - camera.projection;
  // }

  // ---- :D

  Vector3 placeAdd{0, 0, 0};
  for (float cRad = 0; cRad < reach + REACH_STEP; cRad += REACH_STEP) {
    Vector3 lookAtf{dSin(rot_y) * dCos(rot_x) * cRad + camera.position.x,
                    dCos(rot_y) * cRad + camera.position.y,
                    dSin(rot_y) * dSin(rot_x) * cRad + camera.position.z};
    looking_at = (Vector3){roundf(lookAtf.x), roundf(lookAtf.y), roundf(lookAtf.z)};

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

    // if (!onMapV(lookAt)) {
    //   lookAt.x = -1;
    //   placeAdd = (Vector3){0, 0, 0};
    //   break;
    // }

    if (cRad >= reach) {
      looking_at.x = -1;
      placeAdd = (Vector3){0, 0, 0};
    }
  }

  if (IsKeyPressed(KEY_P)) {
    fast_place = !fast_place;
  }

  if (IsKeyPressed(KEY_T)) {
    player_cube.pos = tpPoint;
    notify(TextFormat(
      "Teleported to: %.1f, %.1f, %.1f", tpPoint.x, tpPoint.y, tpPoint.z
    ));
  }

  Vector3 placeAt = Vector3Add(looking_at, placeAdd);

  if (IsKeyPressed(KEY_V)) {
    Vector3 vel {dSin(rot_y)*dCos(rot_x), dCos(rot_y), dSin(rot_y)*dSin(rot_x)};
    Vector3 pos = player_cube.pos + player_cube.size/2 + vel*2;
    vel *= 2;

    falling_blocks.push_back((FallingBlock){ vel, pos, cur_type, });
  }

  // if (IsKeyDown(KEY_B)) {
  //   Vector3 vel {dSin(rot_y)*dCos(rot_x), dCos(rot_y), dSin(rot_y)*dSin(rot_x)};
  //   Vector3 pos = player_cube.pos + player_cube.size/2 + vel*2;
  //   vel *= 2;
  //
  //   fallingBlocks.push_back((FallingBlock){ vel, pos, bt_wool_red });
  // }

  if (IsKeyDown(KEY_X) && on_map(looking_at) && !is_empty(looking_at)) {
    cur_type = get_at(looking_at);
  }

  bool action_break = (fast_place ? IsKeyDown : IsKeyPressed)(KEY_U);
  if (action_break && !is_empty((int)looking_at.x, (int)looking_at.y, (int)looking_at.z) &&
      on_map(looking_at) && on_map(looking_at)) {
    // set to air (delete)
    set_at(looking_at, bt_air);
    PlaySound(sounds[sid_break]);
  }

  bool action_place = (fast_place ? IsKeyDown : IsKeyPressed)(KEY_O);
  if (action_place && on_map(placeAt) && is_empty(placeAt) && (flying || !player_cube.collide(blockCubeV(placeAt)))) {
    set_at(placeAt, cur_type);
    // fallingBlocks.push_back({0, {placeAt.x, placeAt.y+0.5f, placeAt.z},
    // cType});
    PlaySound(sounds[sid_place]);
  }

  if (on_map(placeAt) && IsKeyPressed(KEY_I)) {
    genTree(placeAt.x, placeAt.y, placeAt.z);
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
          [(int)looking_at.y % chunkSize]
          [(int)looking_at.z]
          = cur_type;
  }

  if (IsKeyPressed(KEY_F2)) {
    takeScreenshot();
  }

  // toggle flying
  if (IsKeyPressed(KEY_F)) {
    flying = !flying;
  }

  const bool ctrl = IsKeyDown(KEY_LEFT_CONTROL);

  // save world
  if (ctrl && IsKeyPressed(KEY_S)) {
    std::string fName = guiInputTxt("save as");
    if (fName != "" && saveWorld(fName, player_cube) == 0) {
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

  // Generate world
  if (ctrl && IsKeyPressed(KEY_G)) {
    genWorld();
  }

  // clear world
  if (ctrl && IsKeyPressed(KEY_C)) {
    clearWorld();
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
      char t1, t2;

      try {
        t1 = std::stoi(guiInputTxt("from"));
        t2 = std::stoi(guiInputTxt("to"));
      } catch (std::exception &e) {
        printf("[ERROR] %s", e.what());
        notify(TextFormat("[ERROR] %s", e.what()));
      }

      for (int c = 0; c < maxChunks; c++) {
      for (int x = 0; x < chunkSize; x++) {
      for (int y = 0; y < chunkSize; y++) {
      for (int z = 0; z < chunkSize; z++) {
        if (blocks[c][x][y][z] == t1) {
          blocks[c][x][y][z] = t2;
        }
      }}}}
    }

    else if (command == "tp") {
      try {
        std::vector<std::string> input = splitStr(
          guiInputTxt("teleport to: [x] [y] [z]"),
          ' '
        );
        if (input.size() == 3) {
          player_cube.pos = {
            (float) std::stoi(input[0]),
            (float) std::stoi(input[1]),
            (float) std::stoi(input[2]),
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

        for (int c = 0; c < maxChunks; c++) {
          for (int y = 0; y < chunkSize; y++) {
            for (int x = 0; x < chunkSize; x++) {
              for (int z = 0; z < chunkSize; z++) {

                if (blocks[c][x][y][z] == rbid) {
                  blocks[c][x][y][z] = rand() % blockTypeCount;
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
  const float cx = camera.position.x, cy = camera.position.y, cz = camera.position.z;
  const bool lt = false;

  return {
      (cx < x && (is_empty(x - 1, y, z) || (lt && blockData[get_at(x - 1, y, z)].translucent))),
      (cx > x && (is_empty(x + 1, y, z) || (lt && blockData[get_at(x + 1, y, z)].translucent))),
      (cy < y && (is_empty(x, y - 1, z) || (y > 0 && lt && blockData[get_at(x, y - 1, z)].translucent))),
      (cy > y && (is_empty(x, y + 1, z) || (y < chunkSize * maxChunks && lt && blockData[get_at(x, y + 1, z)].translucent))),
      (cz < z && (is_empty(x, y, z - 1) || (z > 0 && lt && blockData[get_at(x, y, z - 1)].translucent))),
      (cz > z && (is_empty(x, y, z + 1) || (z < chunkSize && lt && blockData[get_at(x, y, z + 1)].translucent))),
  };
}

c4v getC4v(int x, int y, int z, int faceN) {
  return (smoothLighting ? genC4v(x, y, z, faceN) : (c4v){1, 1, 1, 1});
}

void draw_block(int x, int y, int z) {
  static const float
    b0 = 1.0f, // top
    b1 = 0.9f, // left, right
    b2 = 0.8f, // front, back
    b3 = 0.7f; // bottom

  static int lastT, t;
  lastT = -1;

  if (get_at(x, y, z) == bt_air) {
    return;
  }

  Faces faces = getFaces(x, y, z);
  if (
    !faces.back && !faces.front
    && !faces.left && !faces.right
    && !faces.top && !faces.bottom
  ) {
    return;
  }

  int type = get_at(x, y, z);

  // drawCubeTextureFaces({(float) x, (float) y, (float) z}, faces,
  // type);

  /*  c0 = getLOf(x,y,z,0),
      c1 = getLOf(x,y,z,1),
      c2 = getLOf(x,y,z,2),
      c3 = getLOf(x,y,z,3),
      c4 = getLOf(x,y,z,4),
      c5 = getLOf(x,y,z,5);*/

  int fx = 0;

  // if (isFlippable(type)) {
  //   srand((unsigned int)(x % 8 + y % 8 + z % 8));
  //   srand(rand());
  //   fx = rand() % 2;
  // }

  if (1 && type == bt_stone && is_empty(x, y * chunkSize + 1, z)) {
    DrawBillboard(
        camera, textures[blockData[bt_wool_red].sides[1]],
        {(float)x, (float)(y + 1), (float)z}, 1,
        WHITE);
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

    color4ubGF(vc.tl * b0);
    rlTexCoord2f(0.0f + fx, 0.0f);
    rlVertex3f(x - 0.5f, y + 0.5f, z - 0.5f); // Top Left of the Quad

    color4ubGF(vc.br * b0);
    rlTexCoord2f(0.0f + fx, 1.0f);
    rlVertex3f(x - 0.5f, y + 0.5f,
               z + 0.5f); // Bottom Left Of The Quad

    color4ubGF(vc.tr * b0);
    rlTexCoord2f(1.0f - fx, 1.0f);
    rlVertex3f(x + 0.5f, y + 0.5f,
               z + 0.5f); // Bottom Right Of The Quad

    color4ubGF(vc.bl * b0);
    rlTexCoord2f(1.0f - fx, 0.0f);
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
    rlTexCoord2f(1.0f - fx, 1.0f);
    rlVertex3f(x + 0.5f, y - 0.5f,
               z - 0.5f); // Bottom Right Of The Texture and Quad

    color4ubGF(vc.tl * b1);
    rlTexCoord2f(1.0f - fx, 0.0f);
    rlVertex3f(x + 0.5f, y + 0.5f,
               z - 0.5f); // Top Right Of The Texture and Quad

    color4ubGF(vc.bl * b1);
    rlTexCoord2f(0.0f + fx, 0.0f);
    rlVertex3f(x + 0.5f, y + 0.5f,
               z + 0.5f); // Top Left Of The Texture and Quad

    color4ubGF(vc.tr * b1);
    rlTexCoord2f(0.0f + fx, 1.0f);
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
    rlTexCoord2f(0.0f + fx, 1.0f);
    rlVertex3f(x - 0.5f, y - 0.5f,
               z - 0.5f); // Bottom Left Of The Texture and Quad

    color4ubGF(vc.tr * b1);
    rlTexCoord2f(1.0f - fx, 1.0f);
    rlVertex3f(x - 0.5f, y - 0.5f,
               z + 0.5f); // Bottom Right Of The Texture and Quad

    color4ubGF(vc.bl * b1);
    rlTexCoord2f(1.0f - fx, 0.0f);
    rlVertex3f(x - 0.5f, y + 0.5f,
               z + 0.5f); // Top Right Of The Texture and Quad

    color4ubGF(vc.tl * b1);
    rlTexCoord2f(0.0f + fx, 0.0f);
    rlVertex3f(x - 0.5f, y + 0.5f,
               z - 0.5f); // Top Left Of The Texture and Quad
  }

  // Front Face

  if (faces.back) {
    // color4ubG(c*b2, 255, 0);
    rlNormal3f(0.0f, 0.0f, 1.0f); // Normal Pointing Towards Viewer

    c4v vc = getC4v(x, y, z, face_back);

    color4ubGF(vc.br * b2);
    rlTexCoord2f(0.0f + fx, 1.0f);
    rlVertex3f(x - 0.5f, y - 0.5f,
               z + 0.5f); // Bottom Left Of The Texture and Quad

    color4ubGF(vc.tr * b2);
    rlTexCoord2f(1.0f - fx, 1.0f);
    rlVertex3f(x + 0.5f, y - 0.5f,
               z + 0.5f); // Bottom Right Of The Texture and Quad

    color4ubGF(vc.bl * b2);
    rlTexCoord2f(1.0f - fx, 0.0f);
    rlVertex3f(x + 0.5f, y + 0.5f,
               z + 0.5f); // Top Right Of The Texture and Quad

    color4ubGF(vc.tl * b2);
    rlTexCoord2f(0.0f + fx, 0.0f);
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
    rlTexCoord2f(1.0f - fx, 1.0f);
    rlVertex3f(x - 0.5f, y - 0.5f,
               z - 0.5f); // Bottom Right Of The Texture and Quad

    color4ubGF(vc.tl * b2);
    rlTexCoord2f(1.0f - fx, 0.0f);
    rlVertex3f(x - 0.5f, y + 0.5f,
               z - 0.5f); // Top Right Of The Texture and Quad

    color4ubGF(vc.bl * b2);
    rlTexCoord2f(0.0f + fx, 0.0f);
    rlVertex3f(x + 0.5f, y + 0.5f,
               z - 0.5f); // Top Left Of The Texture and Quad

    color4ubGF(vc.tr * b2);
    rlTexCoord2f(0.0f + fx, 1.0f);
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
    color4ubGF(vc.tl * b3);
    rlTexCoord2f(1.0f - fx, 0.0f);
    rlVertex3f(x - 0.5f, y - 0.5f,
               z - 0.5f); // Top Right Of The Texture and Quad

    color4ubGF(vc.bl * b3);
    rlTexCoord2f(0.0f + fx, 0.0f);
    rlVertex3f(x + 0.5f, y - 0.5f,
               z - 0.5f); // Top Left Of The Texture and Quad

    color4ubGF(vc.tr * b3);
    rlTexCoord2f(0.0f + fx, 1.0f);
    rlVertex3f(x + 0.5f, y - 0.5f,
               z + 0.5f); // Bottom Left Of The Texture and Quad

    color4ubGF(vc.br * b3);
    rlTexCoord2f(1.0f - fx, 1.0f);
    rlVertex3f(x - 0.5f, y - 0.5f,
               z + 0.5f); // Bottom Right Of The Texture and Quad
  }

}

void draw3D() {
  BeginMode3D(camera);

  int oldSeed = rand();

  int cc = (int)round(camera.position.y / chunkSize);

  bool lookUp = (rot_y < 90.0f);

  // minimum
  int k1 = lookUp ? max(cc - 1, 0) : max(cc - render_dist, 0);
  // maximum
  int k2 =
      lookUp ? min(cc + render_dist, maxChunks - 1) : min(cc, maxChunks - 1);

  // rlCheckRenderBatchLimit((blockCount + fallingBlocks.size())*3);
  rlPushMatrix();
  rlBegin(RL_QUADS);

  for (size_t i = 0; i < falling_blocks.size(); i++) {
    FallingBlock& fb = falling_blocks[i];

    if (abs(fb.pos.y - camera.position.y) > render_dist * chunkSize) {
      continue;
    }

    Faces f{
        camera.position.x < fb.pos.x, camera.position.x > fb.pos.x,
        camera.position.y < fb.pos.y, camera.position.y > fb.pos.y,
        camera.position.z < fb.pos.z, camera.position.z > fb.pos.z,
    };

    drawCubeTextureFaces(fb.pos, f, fb.type);
  }

  for (int chunk = k1; chunk < k2 + 1; chunk++) {
  for (int i = 0; i < chunkSize; i++) {
    int y = chunk * chunkSize + i;
  for (int x = 0; x < chunkSize; x++) {
  for (int z = 0; z < chunkSize; z++) {
    draw_block(x, y, z);
  }}}}

  rlEnd();
  rlPopMatrix();

  if (on_map(looking_at)) {
    Color c = BLACK; // ColorFromHSV((float)GetTime()*20.0f, 1.0f, 1.0f);
    // DrawCube(lookAt, 1,1,1, c);
    float las = 1.05f;
    DrawCubeWiresV({looking_at.x, looking_at.y, looking_at.z}, {las, las, las}, c);
  }

  SetRandomSeed(oldSeed);

  EndMode3D();
}

void draw2D() {
  std::vector<std::string> infos {
      TextFormat("Version: %s", VERSION),
      TextFormat("xyz: %.1f %.1f %.1f", camera.position.x, camera.position.y,
                 camera.position.z),
      TextFormat("looking at: %s",
                 (looking_at.x == -1) ? "----"
                                  : TextFormat("%i, %i, %i (%i)", (int)looking_at.x,
                                               (int)looking_at.y, (int)looking_at.z,
                                               get_at(looking_at))),
      blockData[cur_type].name,
      TextFormat("Y velocity: %.2f m/s (%.2f km/h)", vel_y * 20.0f,
                 vel_y * 20.0f * 3.6f),
      TextFormat("Rot (DEG): %.2f, %.2f", rot_x, rot_y),
      TextFormat("Fast place/break: %s", onOff[fast_place].c_str()),
      TextFormat("Smooth lighting: %s", onOff[smoothLighting].c_str()),
      TextFormat("flying: %s", onOff[flying].c_str()),
      TextFormat("falling block count: %i", falling_blocks.size()),
  };

  float fontSize = (float) GetScreenHeight() / 40;
  for (size_t i = 0; i < infos.size(); i++) {
    // printf("%i: %s\n", i, texts[i].c_str());
    DrawTextEx(font, infos[i].c_str(),
               {10, (float)(10 + i * (fontSize * 1.2f))}, fontSize, 2, WHITE);
  }

  for (size_t i = 0; i < notifications.size(); i++) {
    int i2 = notifications.size() - 1 - i;
    unsigned char a = (i == 0)
      ? (unsigned char)(int)(min(max(notification_cooldown, 0.0f), 1.0f) * 255.0f)
      : 255;

    DrawTextC(
      notifications[i].c_str(),
      {(float)GetScreenWidth() * 0.5f, 60 + (float)i2 * 30},
      fontSize,
      (Color){255, 255, 255, a});
  }

  // draw selected block
  const Texture2D& tex = textures[blockData[cur_type].sides[1]];

  float selected_block_size = (float)GetScreenHeight() * 0.1f;
  DrawRectangleRec(
    {
      15,
      (float)GetScreenHeight() - (selected_block_size + 25),
      (float)selected_block_size + 10,
      (float)selected_block_size + 10
    },
    BLACK
  );
  DrawTexturePro(
    tex,
    {0.0f, 0.0f, (float)tex.width, (float)tex.height},
    {
      20, (float)GetScreenHeight() - selected_block_size - 20,
      selected_block_size, selected_block_size
    },
    {0, 0}, 0, WHITE
  );

  // draw crosshair
  drawCrosshair(WHITE);

  DrawFPS(0, 0);

  float keystroke_size = GetScreenWidth() * 0.03f;
  drawKeystrokes({GetScreenWidth() - keystroke_size * 3.3f, 0}, keystroke_size);
}

void draw() {
  BeginDrawing();
  ClearBackground(bgc);

  draw3D();
  draw2D();

  EndDrawing();
}

void init() {
  // logF.open("log.txt");

  // if (!logF) {
  //   printf("failed to open log file.\n");
  //   exit(1);
  // }

  // cLog(0, "==== INIT ====");

  // InitWindow(W, H, "Game");
  InitWindow(1280, 720, "Game");

  SetExitKey(KEY_F4);
  SetWindowState(FLAG_VSYNC_HINT);

  HideCursor();

  font = LoadFont(fontFName);
  initTextures("mc16x");
  // cLog(1, "default texture pack loaded");
  // printf("loadTextures: done :D\n");

  // InitAudioDevice();
  initSounds();

  camera.position = (Vector3){chunkSize / 2.0f, 20.0f, chunkSize / 2.0f};
  camera.target = (Vector3){0.0f, 0.0f, 0.0f};
  camera.up = (Vector3){0.0f, 1.0f, 0.0f};
  camera.fovy = DEF_FOV;
  camera.projection = CAMERA_PERSPECTIVE;


  // ToggleFullscreen();

  // NOTE: seems to cause segfault?
  // genWorld();

  // loadWorld("pagoda", pCube);
  load_world("ch64", player_cube);
}

void deInit() {

  // cLog(0, "==== DE-INIT ====");
  UnloadFont(font);
  // cLog(1, "font unloaded");

  for (int i = 0; i < texC; i++)
    UnloadTexture(textures[i]);

  deInitSounds();
  // cLog(1, "sounds unloaded");

  CloseWindow();
  // cLog(1, "closed without any errors");

  exit(0);
}

int main() {
  init();
  while (running) {

    dt = GetFrameTime();
    draw();

    if (dt < 0.1f)
      update();

    running = !WindowShouldClose() && running;
  }

  deInit();
}

std::string guiInputTxt(std::string displayText) {
  int key;
  std::string txt;

  // bool isTyping = true;

  while (!WindowShouldClose()) {
    BeginDrawing();

    ClearBackground(bgc);

    draw3D();
    draw2D();

    DrawTextC(
      displayText.c_str(),
      {(float)GetScreenWidth()/2, (float)GetScreenHeight()/2 - 40},
      GetScreenHeight() / 10.0f,
      WHITE
    );
    DrawTextC(txt.c_str(), {(float)GetScreenWidth()/2, (float)GetScreenHeight()/2 + 40}, GetScreenHeight()/10.0f, WHITE);

    EndDrawing();

    key = GetCharPressed();

    // input text
    while (key > 0) {
      // 25, 125
      if ((key >= 25) && (key <= 125)) {
        txt.push_back((char)key);

        key = GetCharPressed();
      } else
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && txt.length() > 0) {
      txt.resize(txt.size() - sizeof(char));
    }

    if (IsKeyPressed(KEY_F11))
      ToggleFullscreen();

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
    ClearBackground(bgc);
    draw3D();
    DrawRectangle(100, 100, 100, 100, RED);

    DrawTextC(
      "Paused.",
      {(float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() * 0.5f},
      GetScreenHeight() / 40.0f, WHITE
    );

    EndDrawing();

    if (IsKeyPressed(KEY_ESCAPE)) {
      return;
    }
  }
}

void blockSelectionMenu() {
  if (IsCursorHidden()) {
    ShowCursor();
  }

  bool run = true;

  int cols = 9, rows = (int)ceilf((float)blockTypeCount / (float)cols);
  int sb = GetScreenWidth() / 100;

  float scroll = 0;

  while (!WindowShouldClose() && run) {
    scroll += GetMouseWheelMove() * 50.0f;
    scroll = min(max(scroll, 0), GetScreenHeight() - 100);
    BeginDrawing();
    ClearBackground(bgc);

    draw3D();

    DrawCircle(GetScreenWidth() / 2, GetScreenHeight()/2 + 20, 10, YELLOW);

    float block_type_size = (float)(GetScreenHeight() / 13.5f);

    for (int row = 0; row < rows; row++) {
      for (int col = 0; col < cols; col++) {
        int type = row * cols + col;
        if (type > blockTypeCount - 1)
          break;

        const Texture2D& tex = textures[blockData[type].sides[1]];

        Vector2 center = {
            (float)GetScreenWidth()/2 + (col - cols / 2.0f) * (block_type_size + 10 + sb),
            (float)(100 + row * (block_type_size + 10 + sb) + scroll)};

        DrawCircle(center.x, center.y, 10, RED);

        float outline_width = 5.0f;
        Rectangle rec = {
          center.x - block_type_size*0.5f + outline_width,
          center.y - block_type_size*0.5f + outline_width,
          (float)block_type_size + 2*outline_width,
          (float)block_type_size + 2*outline_width
        };

        DrawRectangleRec(
            rec,
            //{(float)CX-(cTypeShowSize/2+5),(float)H-(cTypeShowSize+25),(float)cTypeShowSize+10,(float)cTypeShowSize+10},
            cur_type == type ? WHITE : BLACK);

        DrawTexturePro(
            tex, {0.0f, 0.0f, (float)tex.width, (float)tex.height},
            {center.x - block_type_size * 0.5f, center.y - block_type_size * 0.5f,
             block_type_size, block_type_size},
            {0.0f, 0.0f}, 0.0f, WHITE
        );

        if (CheckCollisionPointRec(GetMousePosition(), rec)) {
          DrawTextC(blockData[type].name,
                    {center.x, center.y - block_type_size * 0.8f}, GetScreenHeight()/40.0f,
                    WHITE);
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

  if (!IsCursorHidden()) {
    HideCursor();
  }
}
