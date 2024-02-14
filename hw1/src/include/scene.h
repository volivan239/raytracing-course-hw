#pragma once
#include "point.h"
#include "color.h"
#include "figures.h"
#include <string>
#include <vector>
#include <memory>

class Scene {
private:
    Ray getCameraRay(int x, int y) const;
    
public:
    int width, height;
    Color bgColor;
    Point cameraPos, cameraUp, cameraRight, cameraForward;
    float cameraFovX;
    std::vector<std::unique_ptr<Figure>> figures;

    Scene();

    Color getPixel(int x, int y) const;
};