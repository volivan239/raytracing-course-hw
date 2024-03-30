#include "scene.h"
#include <cmath>
#include <random>
#include <algorithm>

std::uniform_real_distribution<float> u01(0.0, 1.0);

Scene::Scene() {}

void Scene::initDistribution() {
    auto lightDistribution = std::make_unique<FiguresMix>(figures);
    std::vector<std::unique_ptr<Distribution>> finalDistributions;
    finalDistributions.push_back(std::make_unique<Cosine>());
    if (!lightDistribution->isEmpty()) {
        finalDistributions.push_back(std::move(lightDistribution));
    }
    distribution = std::make_unique<Mix>(std::move(finalDistributions));
}

void Scene::initBVH() {
    nonPlanes = std::partition(figures.begin(), figures.end(), [](const auto &elem) {
        return elem.type != FigureType::PLANE;
    }) - figures.begin();
    bvh = BVH(figures, nonPlanes);
}

std::optional<std::pair<Intersection, int>> Scene::intersect(const Ray &ray) const {
    std::optional<std::pair<Intersection, int>> bestIntersection = bvh.intersect(figures, ray);
    for (int i = nonPlanes; i < (int) figures.size(); i++) {
        auto intersection_ = figures[i].intersect(ray);
        if (intersection_.has_value()) {
            auto intersection = intersection_.value();
            if (!bestIntersection.has_value() || intersection.t < bestIntersection.value().first.t) {
                bestIntersection = {intersection, i};
            }
        }
    }

    return bestIntersection;
}

Color Scene::getColor(rng_type &rng, const Ray &ray, int recLimit) const {
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
    auto x = ray.o + t * ray.d;

    if (figurePtr->material == Material::DIFFUSE) {
        Vec3 d = distribution->sample(rng, x + 0.0001 * norma, norma);
        if (d.dot(norma) < 0) {
            return figurePtr->emission;
        }
        float pdf = distribution->pdf(x + 0.0001 * norma, norma, d);
        Ray dRay = Ray(x + 0.0001 * d, d);
        return figurePtr->emission + 1. / (PI * pdf) * d.dot(norma) * figurePtr->color * getColor(rng, dRay, recLimit - 1);
    } else if (figurePtr->material == Material::METALLIC) {
        Vec3 reflectedDir = ray.d.normalize() - 2. * norma.dot(ray.d.normalize()) * norma;
        Ray reflected = Ray(ray.o + t * ray.d + 0.0001 * reflectedDir, reflectedDir);
        return figurePtr->emission + figurePtr->color * getColor(rng, reflected, recLimit - 1);
    } else {
        Vec3 reflectedDir = ray.d.normalize() - 2. * norma.dot(ray.d.normalize()) * norma;
        Ray reflected = Ray(ray.o + t * ray.d + 0.0001 * reflectedDir, reflectedDir);
        Color reflectedColor = getColor(rng, reflected, recLimit - 1);

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
        if (u01(rng) < r) {
            return figurePtr->emission + reflectedColor;
        }

        float cosTheta2 = sqrt(1 - sinTheta2 * sinTheta2);
        Vec3 refractedDir = eta1 / eta2 * (-1. * l) + (eta1 / eta2 * norma.dot(l) - cosTheta2) * norma;
        Ray refracted = Ray(ray.o + t * ray.d + 0.0001 * refractedDir, refractedDir);
        Color refractedColor = getColor(rng, refracted, recLimit - 1);
        if (!is_inside) {
            refractedColor = refractedColor * figurePtr->color;
        }
        return figurePtr->emission + refractedColor;
    }
}

Color Scene::getPixel(rng_type &rng, int x, int y) const {
    Color color {0, 0, 0};
    for (int _ = 0; _ < samples; _++) {
        float nx = x + u01(rng);
        float ny = y + u01(rng);
        color = color + getColor(rng, getCameraRay(nx, ny), rayDepth);
    }
    return 1.0 / samples * color;
}

Ray Scene::getCameraRay(float x, float y) const {
    float tanFovX = tan(cameraFovX / 2);
    float tanFovY = tanFovX * height / width;

    float nx = tanFovX * (2 * x / width - 1);
    float ny = tanFovY * (2 * y / height - 1);

    return {cameraPos, nx * cameraRight - ny * cameraUp + cameraForward};
}