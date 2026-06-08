#include "util.hpp"

std::vector<std::string> split_str(const std::string& str, char delimiter) {
    std::vector<std::string> strVec = {};
    std::string nStr = "";

    for (char ch : str) {
        if (ch == delimiter) {
            if (nStr.size() == 0) {
                continue;
            }

            strVec.push_back(nStr);
            nStr = "";
        } else {
            nStr.push_back(ch);
        }
    }
    strVec.push_back(nStr);
    return strVec;
}

void take_screenshot() {
    Image screenImage = LoadImageFromScreen();
    int screenShotId = 1;

    while (FileExists(TextFormat("screenshots/%i.png", screenShotId))) {
        screenShotId++;
    }
    ExportImage(screenImage, TextFormat("screenshots/%i.png", screenShotId));
    UnloadImage(screenImage);
}

void DrawTextC(Font font, const char *text, Vector2 center, float fontSize, Color color) {
    DrawTextEx(font, text,
               {center.x - MeasureTextEx(font, text, fontSize, 0).x * 0.5f,
                center.y - fontSize / 2},
               fontSize, 0, color);
}
