#pragma once

#include <math.h>
#include <string>
#include <vector>

#include <raylib.h>
#include <raymath.h>

#ifndef dCos
#define dCos(x) (cosf(x * PI / 180.0f))
#endif

#ifndef dSin
#define dSin(x) (sinf(x * PI / 180.0f))
#endif

struct Vector3i {
    int x, y, z;
    Vector3 to_raylib() {
        return {(float)x, (float)y, (float)z};
    }
    static Vector3i from_raylib(Vector3 vec) {
        return {(int)std::round(vec.x), (int)std::round(vec.y), (int)std::round(vec.z)};
    }
    Vector3i operator+(Vector3i other) {
        return {x + other.x, y + other.y, z + other.z};
    }
};

std::vector<std::string> split_str(const std::string& str, char delimiter);

// draw centered text
void DrawTextC(Font font, const char *text, Vector2 center, float fontSize, Color color);

void take_screenshot();

enum faceEnum {
    face_left = 0, // X -
    face_right,    // X +
    face_bottom,   // Y -
    face_top,      // Y +
    face_front,    // Z -
    face_back      // Z +
};

struct c4v {
    float tl, bl, tr, br;
};

struct Cube {
    Vector3 pos, size;

    // top left front
    Vector3 getTLF() {
        return {pos.x - size.x / 2, pos.y - size.y / 2, pos.z - size.z / 2};
    }

    // bottom right back
    Vector3 getBRB() {
        return {pos.x + size.x / 2, pos.y + size.y / 2, pos.z + size.z / 2};
    }

    bool collide(Cube c2) {

        Vector3 tl = getTLF(), c2tl = c2.getTLF();

        return (tl.x < (c2tl.x + c2.size.x) && (tl.x + size.x) > c2tl.x &&

                tl.y < (c2tl.y + c2.size.y) && (tl.y + size.y) > c2tl.y &&

                tl.z < (c2tl.z + c2.size.z) && (tl.z + size.z) > c2tl.z);
    }

    Cube movedCopy(Vector3 delta) {
        return Cube{pos + delta, size};
    }
};
