#pragma once

#include <math.h>
#include <string>
#include <vector>

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#ifndef dCos
#define dCos(x) (cosf(x * PI / 180.0f))
#endif

#ifndef dSin
#define dSin(x) (sinf(x * PI / 180.0f))
#endif

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

    void setCenter(Vector3 center) {
        pos.x = center.x - size.x * 0.5f;
        pos.y = center.y - size.y * 0.5f;
        pos.z = center.z - size.z * 0.5f;
    }

    bool collide(Cube c2) {

        Vector3 tl = getTLF(), c2tl = c2.getTLF();

        return (tl.x < (c2tl.x + c2.size.x) && (tl.x + size.x) > c2tl.x &&

                tl.y < (c2tl.y + c2.size.y) && (tl.y + size.y) > c2tl.y &&

                tl.z < (c2tl.z + c2.size.z) && (tl.z + size.z) > c2tl.z);
    }

    void draw(Color color) {
        DrawCubeV(pos, size, color);
    }

    Cube movedCopy(float x, float y, float z) {
        return Cube{Vector3Add(pos, {x, y, z}), size};
    }

    Cube movedCopyV(Vector3 delta) {
        return Cube{Vector3Add(pos, delta), size};
    }
};
