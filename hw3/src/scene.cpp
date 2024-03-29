#include "scene.h"
#include <cmath>
#include <random>

static std::minstd_rand rnd;
static std::uniform_real_distribution<float> u01(0.0, 1.0);
static std::normal_distribution<float> n01(0.0, 1.0);

Scene::Scene() {}

std::optional<std::pair<Intersection, int>> Scene::intersect(const Ray &ray, float tmax) const {
    Intersection bestIntersection;
    int bestPos = -1;
    for (int i = 0; i < (int) figures.size(); i++) {
        auto intersection_ = figures[i].intersect(ray);
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
    auto figurePtr = figures.begin() + figurePos;

    if (figurePtr->material == Material::DIFFUSE) {
        Vec3 w = Vec3 {n01(rnd), n01(rnd), n01(rnd)}.normalize();
        if (w.dot(norma) < 0) {
            w = -1. * w;
        }
        Ray wRay = Ray(ray.o + t * ray.d + 0.0001 * w, w);
        return figurePtr->emission + 2 * w.dot(norma) * figurePtr->color * getColor(wRay, recLimit - 1);
    } else if (figurePtr->material == Material::METALLIC) {
        Vec3 reflectedDir = ray.d.normalize() - 2. * norma.dot(ray.d.normalize()) * norma;
        Ray reflected = Ray(ray.o + t * ray.d + 0.0001 * reflectedDir, reflectedDir);
        return figurePtr->emission + figurePtr->color * getColor(reflected, recLimit - 1);
    } else {
        Vec3 reflectedDir = ray.d.normalize() - 2. * norma.dot(ray.d.normalize()) * norma;
        Ray reflected = Ray(ray.o + t * ray.d + 0.0001 * reflectedDir, reflectedDir);
        Color reflectedColor = getColor(reflected, recLimit - 1);

        float eta1 = 1., eta2 = figurePtr->ior;
        if (is_inside) {
            std::swap(eta1, eta2);
        }

        Vec3 l = -1. * ray.d.normalize();
        float sinTheta2 = eta1 / eta2 * sqrt(1 - norma.dot(l) * norma.dot(l));
        if (fabs(sinTheta2) > 1.) {
            return figurePtr->emission + reflectedColor;
        }

        float r0 = pow((eta1 - eta2) / (eta1 + eta2), 2.);
        float r = r0 + (1 - r0) * pow(1 - norma.dot(l), 5.);
        if (u01(rnd) < r) {
            return figurePtr->emission + reflectedColor;
        }

        float cosTheta2 = sqrt(1 - sinTheta2 * sinTheta2);
        Vec3 refractedDir = eta1 / eta2 * (-1. * l) + (eta1 / eta2 * norma.dot(l) - cosTheta2) * norma;
        Ray refracted = Ray(ray.o + t * ray.d + 0.0001 * refractedDir, refractedDir);
        Color refractedColor = getColor(refracted, recLimit - 1);
        if (!is_inside) {
            refractedColor = refractedColor * figurePtr->color;
        }
        return figurePtr->emission + refractedColor;
    }
}

Color Scene::getPixel(int x, int y) const {
    Color color {0, 0, 0};
    for (int _ = 0; _ < samples; _++) {
        float nx = x + u01(rnd);
        float ny = y + u01(rnd);
        color = color + getColor(getCameraRay(nx, ny), rayDepth);
    }
    return 1.0 / samples * color;
}

Ray Scene::getCameraRay(float x, float y) const {
    float tanFovX = tan(cameraFovX / 2);
    float tanFovY = tanFovX * height / width;

    float nx = tanFovX * (2 * (x + 0.5) / width - 1);
    float ny = tanFovY * (2 * (y + 0.5) / height - 1);

    return {cameraPos, nx * cameraRight - ny * cameraUp + cameraForward};
}