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

    if (figurePtr->material.material == Material::DIFFUSE) {
        Vec3 d = distribution.sample(u01, n01, rng, x + eps * shadingNorma, shadingNorma);
        if (d.dot(shadingNorma) < 0) {
            return figurePtr->material.emission;
        }
        float pdf = distribution.pdf(x + eps * shadingNorma, shadingNorma, d);
        Ray dRay = Ray(x + eps * d, d);
        return figurePtr->material.emission + 1. / (PI * pdf) * d.dot(shadingNorma) * figurePtr->material.color * getColor(u01, n01, rng, dRay, recLimit - 1);
    } else if (figurePtr->material.material == Material::METALLIC) {
        Vec3 reflectedDir = ray.d.normalize() - 2. * shadingNorma.dot(ray.d.normalize()) * shadingNorma;
        Ray reflected = Ray(ray.o + t * ray.d + eps * reflectedDir, reflectedDir);
        return figurePtr->material.emission + figurePtr->material.color * getColor(u01, n01, rng, reflected, recLimit - 1);
    } else {
        Vec3 reflectedDir = ray.d.normalize() - 2. * shadingNorma.dot(ray.d.normalize()) * shadingNorma;
        Ray reflected = Ray(ray.o + t * ray.d + eps * reflectedDir, reflectedDir);
        Color reflectedColor = getColor(u01, n01, rng, reflected, recLimit - 1);

        float eta1 = 1., eta2 = figurePtr->material.ior;
        if (is_inside) {
            std::swap(eta1, eta2);
        }

        Vec3 l = -1. * ray.d.normalize();
        float sinTheta2 = eta1 / eta2 * sqrt(1 - shadingNorma.dot(l) * shadingNorma.dot(l));
        if (fabs(sinTheta2) > 1.) {
            return figurePtr->material.emission + reflectedColor;
        }

        float r0 = pow((eta1 - eta2) / (eta1 + eta2), 2.);
        float r = r0 + (1 - r0) * pow(1 - shadingNorma.dot(l), 5.);
        if (u01(rng) < r) {
            return figurePtr->material.emission + reflectedColor;
        }

        float cosTheta2 = sqrt(1 - sinTheta2 * sinTheta2);
        Vec3 refractedDir = eta1 / eta2 * (-1. * l) + (eta1 / eta2 * shadingNorma.dot(l) - cosTheta2) * shadingNorma;
        Ray refracted = Ray(ray.o + t * ray.d + eps * refractedDir, refractedDir);
        Color refractedColor = getColor(u01, n01, rng, refracted, recLimit - 1);
        if (!is_inside) {
            refractedColor = refractedColor * figurePtr->material.color;
        }
        return figurePtr->material.emission + refractedColor;
    }
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

    return {cameraPos, nx * cameraRight - ny * cameraUp + cameraForward};
}