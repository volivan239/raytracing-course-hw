#pragma once
#include "vec3.h"
#include "color.h"
#include "primitives.h"
#include <string>
#include <vector>
#include <memory>

class Scene {
private:
    Ray getCameraRay(float x, float y) const;
    std::optional<std::pair<Intersection, int>> intersect(const Ray &ray, float tmax = 1. / 0.) const;
    Color getColor(const Ray &ray, int recLimit) const;

public:
    int samples;
    int rayDepth;
    int width, height;
    Color bgColor = {0, 0, 0};
    Vec3 cameraPos, cameraUp, cameraRight, cameraForward;
    float cameraFovX;
    std::vector<Figure> figures;

    Scene();

    Color getPixel(int x, int y) const;
};