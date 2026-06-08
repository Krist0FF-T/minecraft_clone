#include "draw.hpp"

#include <array>
#include <string>

#include <rlgl.h>

#include "blocktypes.hpp"

Texture2D block_textures[TEXTURE_COUNT];

void init_textures(const char *texture_pack) {
    for (int i = 0; i < TEXTURE_COUNT; i++) {
        std::string cTp =
            (FileExists(TextFormat("texturePacks/%s/%s.png", texture_pack,
                                   TEXTURE_NAMES[i])))
            ? texture_pack
            : "default";

        // if(!FileExists(TextFormat("texturePacks/%s/%s.png", tpName,
        // tNames[i].c_str()))) {
        //     printf("%s not exsits\n", tNames[i].c_str());
        // }

        UnloadTexture(block_textures[i]);
        block_textures[i] = LoadTexture(TextFormat("texturePacks/%s/%s.png",
                                             cTp.c_str(), TEXTURE_NAMES[i]));
        GenTextureMipmaps(&block_textures[i]);
        SetTextureFilter(block_textures[i], TEXTURE_FILTER_POINT);
    }
}

void color4ubGF(float g) {
    unsigned char ug = (unsigned char)(g * 255.0f);
    rlColor4ub(ug, ug, ug, 255);
}

void drawCubeTextureFaces(Vector3 position, Faces faces, int type) {

    if (!faces.back && !faces.bottom && !faces.front && !faces.left &&
        !faces.right && !faces.top) {
        return;
    }

    float x = position.x;
    float y = position.y;
    float z = position.z;

    float width = 1.0f;
    float height = 1.0f;
    float length = 1.0f;

    float b0 = 1.0f; // top
    float b1 = 0.9f; // left, right
    float b2 = 0.8f; // front, back
    float b3 = 0.7f; // bottom

    rlCheckRenderBatchLimit(3 *
                                (faces.back + faces.bottom + faces.front +
                                 faces.left + faces.right + faces.top) +
                            100);

    int t = 0, lastT = t;

    // 0: left
    // 1: right
    // 2: bottom
    // 3: top
    // 4: front
    // 5: back

    // Top Face
    if (faces.top) {
        color4ubGF(b0);
        t = block_textures[blockData[type].sides[0]].id;
        if (lastT != t) {
            rlSetTexture(block_textures[blockData[type].sides[0]].id);
            lastT = t;
        }

        rlNormal3f(0.0f, 1.0f, 0.0f); // Normal Pointing Up
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(x - width / 2, y + height / 2,
                   z - length / 2); // Top Left Of The Texture and Quad
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(x - width / 2, y + height / 2,
                   z + length / 2); // Bottom Left Of The Texture and Quad
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(x + width / 2, y + height / 2,
                   z + length / 2); // Bottom Right Of The Texture and Quad
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(x + width / 2, y + height / 2,
                   z - length / 2); // Top Right Of The Texture and Quad
    }

    if (faces.left || faces.right || faces.front || faces.bottom) {
        t = block_textures[blockData[type].sides[1]].id;
        if (t != lastT) {
            rlSetTexture(t);
            lastT = t;
        }
        color4ubGF(b1);
    }

    // Right face
    if (faces.right) {
        rlNormal3f(1.0f, 0.0f, 0.0f); // Normal Pointing Right
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(x + width / 2, y - height / 2,
                   z - length / 2); // Bottom Right Of The Texture and Quad
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(x + width / 2, y + height / 2,
                   z - length / 2); // Top Right Of The Texture and Quad
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(x + width / 2, y + height / 2,
                   z + length / 2); // Top Left Of The Texture and Quad
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(x + width / 2, y - height / 2,
                   z + length / 2); // Bottom Left Of The Texture and Quad
    }

    // Left Face
    if (faces.left) {
        rlNormal3f(-1.0f, 0.0f, 0.0f); // Normal Pointing Left
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(x - width / 2, y - height / 2,
                   z - length / 2); // Bottom Left Of The Texture and Quad
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(x - width / 2, y - height / 2,
                   z + length / 2); // Bottom Right Of The Texture and Quad
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(x - width / 2, y + height / 2,
                   z + length / 2); // Top Right Of The Texture and Quad
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(x - width / 2, y + height / 2,
                   z - length / 2); // Top Left Of The Texture and Quad
    }

    color4ubGF(b2);
    // Front Face
    if (faces.back) {
        rlNormal3f(0.0f, 0.0f, 1.0f); // Normal Pointing Towards Viewer
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(x - width / 2, y - height / 2,
                   z + length / 2); // Bottom Left Of The Texture and Quad
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(x + width / 2, y - height / 2,
                   z + length / 2); // Bottom Right Of The Texture and Quad
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(x + width / 2, y + height / 2,
                   z + length / 2); // Top Right Of The Texture and Quad
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(x - width / 2, y + height / 2,
                   z + length / 2); // Top Left Of The Texture and Quad
    }

    // Back Face
    if (faces.front) {
        rlNormal3f(0.0f, 0.0f, -1.0f); // Normal Pointing Away From Viewer
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(x - width / 2, y - height / 2,
                   z - length / 2); // Bottom Right Of The Texture and Quad
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(x - width / 2, y + height / 2,
                   z - length / 2); // Top Right Of The Texture and Quad
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(x + width / 2, y + height / 2,
                   z - length / 2); // Top Left Of The Texture and Quad
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(x + width / 2, y - height / 2,
                   z - length / 2); // Bottom Left Of The Texture and Quad
    }

    color4ubGF(b3);

    // Bottom Face
    if (faces.bottom) {
        t = block_textures[blockData[type].sides[1]].id;
        if (lastT != t) {
            rlSetTexture(t);
            lastT = t;
        }
        rlNormal3f(0.0f, -1.0f, 0.0f); // Normal Pointing Down
        rlTexCoord2f(1.0f, 1.0f);
        rlVertex3f(x - width / 2, y - height / 2,
                   z - length / 2); // Top Right Of The Texture and Quad
        rlTexCoord2f(0.0f, 1.0f);
        rlVertex3f(x + width / 2, y - height / 2,
                   z - length / 2); // Top Left Of The Texture and Quad
        rlTexCoord2f(0.0f, 0.0f);
        rlVertex3f(x + width / 2, y - height / 2,
                   z + length / 2); // Bottom Left Of The Texture and Quad
        rlTexCoord2f(1.0f, 0.0f);
        rlVertex3f(x - width / 2, y - height / 2,
                   z + length / 2); // Bottom Right Of The Texture and Quad
    }
}

void drawKeyStroke(Vector2 pos, int key, float size) {
    DrawRectangleV(pos, {size, size}, IsKeyDown(key) ? RED : BLUE);
}

void drawKeystrokes(Vector2 pos, float size) {
    // W
    drawKeyStroke({pos.x + size * 1.1f, pos.y}, KEY_W, size);
    // A
    drawKeyStroke({pos.x, pos.y + size * 1.1f}, KEY_A, size);
    // S
    drawKeyStroke({pos.x + size * 1.1f, pos.y + size * 1.1f}, KEY_S, size);
    // D
    drawKeyStroke({pos.x + size * 2 * 1.1f, pos.y + size * 1.1f}, KEY_D, size);
}
