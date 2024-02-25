#include "scene.h"
#include <assert.h>
#include <math.h>
#include <iostream>

Scene::Scene() {}

std::optional<std::pair<Intersection, int>> Scene::intersect(const Ray &ray, float tmax) const {
    Intersection bestIntersection;
    int bestPos = -1;
    for (int i = 0; i < (int) figures.size(); i++) {
        auto intersection_ = figures[i]->intersect(ray);
        if (intersection_.has_value()) {
            auto intersection = intersection_.value();
            if (intersection.t <= tmax && (bestPos == -1 || intersection.t < bestIntersection.t)) {
                bestIntersection = intersection;
                bestPos = i;
            }
        }
    }

    if (bestPos == -1) {
        return {};
    }
    // std::cerr << bestPos << ' ' << figures.size() << std::endl;
    return {std::make_pair(bestIntersection, bestPos)};
}

Color Scene::getColor(const Ray &ray, int recLimit) const {
    if (recLimit == 0) {
        return {0., 0., 0.};
    }

    auto intersection_ = intersect(ray);
    if (!intersection_.has_value()) {
        return bgColor;
    }

    auto [intersection, figurePos] = intersection_.value();
    auto [t, norma, is_inside] = intersection;

    if (figures[figurePos]->material == Material::DIFFUSE) {
        Color color = ambientLight;
        for (const auto &lightSource : lightSources) {
            Vec3 p = ray.o + t * ray.d;
            auto [l, c, tmax] = lightSource->lightAt(p);
            float reflected = l.dot(norma);
            if (reflected >= 0 && !intersect(Ray(p + 0.0001 * l, l), tmax).has_value()) {
                color = color + reflected * c;
            }
        }
        return color * figures[figurePos]->color;
    } else if (figures[figurePos]->material == Material::METALLIC) {
        Vec3 reflectedDir = ray.d.normalize() - 2. * norma.dot(ray.d.normalize()) * norma;
        Ray reflected = Ray(ray.o + t * ray.d + 0.0001 * reflectedDir, reflectedDir);
        return figures[figurePos]->color * getColor(reflected, recLimit - 1);
    } else {
        Vec3 reflectedDir = ray.d.normalize() - 2. * norma.dot(ray.d.normalize()) * norma;
        Ray reflected = Ray(ray.o + t * ray.d + 0.0001 * reflectedDir, reflectedDir);
        Color reflectedComponent = getColor(reflected, recLimit - 1);

        float eta1 = 1., eta2 = figures[figurePos]->ior;
        if (is_inside) {
            std::swap(eta1, eta2);
        }

        Vec3 l = -1. * ray.d.normalize();
        float sinTheta2 = eta1 / eta2 * sqrt(1 - norma.dot(l) * norma.dot(l));
        if (fabs(sinTheta2) > 1.) {
            return reflectedComponent;
        }

        float cosTheta2 = sqrt(1 - sinTheta2 * sinTheta2);
        Vec3 refractedDir = eta1 / eta2 * (-1. * l) + (eta1 / eta2 * norma.dot(l) - cosTheta2) * norma;
        Ray refracted = Ray(ray.o + t * ray.d + 0.0001 * refractedDir, refractedDir);
        Color refractedComponent = getColor(refracted, recLimit - 1);
        if (!is_inside) {
            refractedComponent = refractedComponent * figures[figurePos]->color;
        }

        float r0 = pow((eta1 - eta2) / (eta1 + eta2), 2.);
        float r = r0 + (1 - r0) * pow(1 - norma.dot(l), 5.);
        return r * reflectedComponent + (1 - r) * refractedComponent;
    }
}

Color Scene::getPixel(int x, int y) const {
    return getColor(getCameraRay(x, y), rayDepth);
}

Ray Scene::getCameraRay(int x, int y) const {
    float tanFovX = tan(cameraFovX / 2);
    float tanFovY = tanFovX * height / width;

    float nx = tanFovX * (2 * (x + 0.5) / width - 1);
    float ny = tanFovY * (2 * (y + 0.5) / height - 1);

    return {cameraPos, nx * cameraRight - ny * cameraUp + cameraForward};
}