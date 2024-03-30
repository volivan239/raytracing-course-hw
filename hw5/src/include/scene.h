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
    std::unique_ptr<Mix> distribution;
    int nonPlanes;

    Ray getCameraRay(float x, float y) const;
    std::optional<std::pair<Intersection, int>> intersect(const Ray &ray) const;
    Color getColor(rng_type &rng, const Ray &ray, int recLimit) const;

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

    Color getPixel(rng_type &rng, int x, int y) const;
    void initDistribution();
    void initBVH();
};