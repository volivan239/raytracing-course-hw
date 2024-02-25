#pragma once
#include "vec3.h"
#include "color.h"
#include "primitives.h"
#include "light_source.h"
#include <string>
#include <vector>
#include <memory>

class Scene {
private:
    Ray getCameraRay(int x, int y) const;
    std::optional<std::pair<Intersection, int>> intersect(const Ray &ray, float tmax = 1. / 0.) const;
    Color getColor(const Ray &ray, int recLimit) const;

public:
    int rayDepth;
    int width, height;
    Color bgColor;
    Color ambientLight;
    Vec3 cameraPos, cameraUp, cameraRight, cameraForward;
    float cameraFovX;
    std::vector<std::unique_ptr<Figure>> figures;
    std::vector<std::unique_ptr<LightSource>> lightSources;

    Scene();

    Color getPixel(int x, int y) const;
};