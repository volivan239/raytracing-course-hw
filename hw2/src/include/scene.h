#pragma once
#include "vec3.h"
#include "color.h"
#include "primitives.h"
#include <string>
#include <vector>
#include <memory>

class Scene {
private:
    Ray getCameraRay(int x, int y) const;

public:
    int width, height;
    int rayDepth;
    Color bgColor;
    Color ambientLight;
    Vec3 cameraPos, cameraUp, cameraRight, cameraForward;
    float cameraFovX;
    std::vector<std::unique_ptr<Figure>> figures;

    Scene();

    Color getPixel(int x, int y) const;
};