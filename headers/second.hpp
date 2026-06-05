#pragma once

#include <vector>
#include <string>
#include <raylib.h>
#include <rlgl.h>
#include <raymath.h>

//#include "third.hpp"

// high size
static const bool hs = true;
// static const int W = (hs ? 1920 : 1280), H = (hs ? 1080 : 720), CX = W/2, CY = H/2;

static const std::vector<std::string> onOff = {
    "OFF",
    "ON"
};

static Font font;
static const char* fontFName = "fonts/retro.ttf";

static const Color bgc{100,180,240,255};

bool vec3eq(Vector3 v1, Vector3 v2) {
    return (v1.x == v2.x && v1.y == v2.y && v1.z == v2.z);
}

void DrawTextC(const char* text, Vector2 center, float fontSize, Color color) {
    DrawTextEx(font, text, {center.x - MeasureTextEx(font, text, fontSize, 0).x*0.5f, center.y-fontSize/2}, fontSize, 0, color);
}

enum faceEnum {
    face_left = 0,     // X -
    face_right,    // X +
    face_bottom,   // Y -
    face_top,      // Y +
    face_front,    // Z -
    face_back     // Z +
};

struct c4v {
    float tl,bl,tr,br;
};


void takeScreenshot() {
    Image screenImage = LoadImageFromScreen();
    int screenShotId = 1;

    while(FileExists(TextFormat("screenshots/%i.png", screenShotId))) {
        screenShotId++;
    }
    ExportImage(screenImage, TextFormat("screenshots/%i.png", screenShotId));
    UnloadImage(screenImage);
}

struct Cube {
    Vector3 pos, size;

    // top left front
    Vector3 getTLF() {
        return {pos.x-size.x/2, pos.y-size.y/2, pos.z-size.z/2};
    }

    // bottom right back
    Vector3 getBRB() {
        return {pos.x+size.x/2, pos.y+size.y/2, pos.z+size.z/2};
    }

    void setCenter(Vector3 center) {
        pos.x = center.x-size.x*0.5f;
        pos.y = center.y-size.y*0.5f;
        pos.z = center.z-size.z*0.5f;
    }

    bool collide(Cube c2) {

        Vector3 tl = getTLF(),
                c2tl = c2.getTLF();

        return (
            tl.x < (c2tl.x+c2.size.x) &&
            (tl.x+size.x) > c2tl.x &&

            tl.y < (c2tl.y+c2.size.y) &&
            (tl.y+size.y) > c2tl.y &&

            tl.z < (c2tl.z+c2.size.z) &&
            (tl.z+size.z) > c2tl.z
        );
    }

    void draw(Color color) {
        DrawCubeV(pos, size, color);
    }

    Cube movedCopy(float x, float y, float z) {
        return Cube{Vector3Add(pos, {x,y,z}), size};
    }

    Cube movedCopyV(Vector3 delta) {
        return Cube{Vector3Add(pos, delta), size};
    }
};
