#include "scene.h"
#include <cmath>
#include <random>
#include <algorithm>

Scene::Scene() {}

void Scene::initDistribution() {
    auto lightDistribution = FiguresMix(figures);
    std::vector<std::variant<Cosine, FiguresMix>> finalDistributions;
    finalDistributions.push_back(Cosine());
    if (!lightDistribution.isEmpty()) {
        finalDistributions.push_back(lightDistribution);
    }
    distribution = Mix(finalDistributions);
}

void Scene::initBVH() {
    bvh = BVH(figures, figures.size());
}

std::optional<std::pair<Intersection, int>> Scene::intersect(const Ray &ray) const {
    return bvh.intersect(figures, ray, {});
}

Color Scene::getColor(std::uniform_real_distribution<float> &u01, std::normal_distribution<float> &n01, rng_type &rng, const Ray &ray, int recLimit) {
    if (recLimit == 0) {
        return {0., 0., 0.};
    }

    auto intersection_ = intersect(ray);
    if (!intersection_.has_value()) {
        return bgColor;
    }

    auto [intersection, figurePos] = intersection_.value();
    auto [t, _, shadingNorma_, is_inside] = intersection;
    auto shadingNorma = shadingNorma_.value();
    auto figurePtr = figures.begin() + figurePos;
    auto x = ray.o + t * ray.d;

    Vec3 d = distribution.sample(u01, n01, rng, x + eps * shadingNorma, shadingNorma);
    Ray dRay = Ray(x + eps * d, d);
    Vec3 brdf = materialModels[figurePtr->materialIndex].brdf(-1.0 * ray.d, dRay.d, shadingNorma);
    if (brdf.x < eps && brdf.y < eps && brdf.z < eps) {
        return figurePtr->material.emission;
    }

    float pdf = distribution.pdf(x + eps * shadingNorma, shadingNorma, d);
    return figurePtr->material.emission + 1.0 / pdf * d.dot(shadingNorma) * getColor(u01, n01, rng, dRay, recLimit - 1) * brdf;
}

Color Scene::getPixel(rng_type &rng, int x, int y) {
    std::uniform_real_distribution<float> u01(0.0, 1.0);
    std::normal_distribution<float> n01(0.0, 1.0);
    Color color {0, 0, 0};
    for (int _ = 0; _ < samples; _++) {
        float nx = x + u01(rng);
        float ny = y + u01(rng);
        color = color + getColor(u01, n01, rng, getCameraRay(nx, ny), rayDepth);
    }
    return 1.0 / samples * color;
}

Ray Scene::getCameraRay(float x, float y) const {
    float tanFovY = tan(cameraFovY / 2);
    float tanFovX = tanFovY * width / height;

    float nx = tanFovX * (2 * x / width - 1);
    float ny = tanFovY * (2 * y / height - 1);

    return {cameraPos, (nx * cameraRight - ny * cameraUp + cameraForward).normalize()};
}