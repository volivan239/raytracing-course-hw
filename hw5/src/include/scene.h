#pragma once
#include "vec3.h"
#include "color.h"
#include "primitives.h"
#include "distributions.h"
#include "bvh.h"
#include <string>
#include <vector>
#include <memory>
#include <random>

typedef std::minstd_rand rng_type;

class Scene {
private:
    Mix distribution;
    int nonPlanes;

    Ray getCameraRay(float x, float y) const;
    std::optional<std::pair<Intersection, int>> intersect(const Ray &ray) const;
    Color getColor(std::uniform_real_distribution<float> &u01, std::normal_distribution<float> &n01, rng_type &rng, const Ray &ray, int recLimit);

public:
    int samples;
    int rayDepth;
    int width, height;
    Color bgColor;
    Vec3 cameraPos, cameraUp, cameraRight, cameraForward;
    float cameraFovX;
    std::vector<Figure> figures;
    BVH bvh;

    Scene();

    Color getPixel(rng_type &rng, int x, int y);
    void initDistribution();
    void initBVH();
};