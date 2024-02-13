#pragma once
#include "point.h"
#include "color.h"
#include <string>

class Scene {
public:
    int width, height;
    Color bgColor;
    Point cameraPos, cameraUp, cameraRight, cmaeraForward;
    float cameraFovX;

    Scene();

    Color getPixel(int x, int y) const;
};