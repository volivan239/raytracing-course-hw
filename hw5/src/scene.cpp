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
    nonPlanes = std::partition(figures.begin(), figures.end(), [](const auto &elem) {
        return elem.type != FigureType::PLANE;
    }) - figures.begin();
    bvh = BVH(figures, nonPlanes);
}

std::optional<std::pair<Intersection, int>> Scene::intersect(const Ray &ray) const {
    std::optional<std::pair<Intersection, int>> bestIntersection = {};
    for (int i = nonPlanes; i < (int) figures.size(); i++) {
        auto intersection_ = figures[i].intersect(ray);
        if (intersection_.has_value()) {
            auto intersection = intersection_.value();
            if (!bestIntersection.has_value() || intersection.t < bestIntersection.value().first.t) {
                bestIntersection = {intersection, i};
            }
        }
    }
    std::optional<float> curBest = {};
    if (bestIntersection.has_value()) {
        curBest = bestIntersection.value().first.t;
    }
    auto bvhIntersection = bvh.intersect(figures, ray, curBest);
    if (bvhIntersection.has_value() && (!bestIntersection.has_value() || bvhIntersection.value().first.t < bestIntersection.value().first.t)) {
        bestIntersection = bvhIntersection;
    }
    return bestIntersection;
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
    auto [t, norma, is_inside] = intersection;
    auto figurePtr = figures.begin() + figurePos;
    auto x = ray.o + t * ray.d;

    if (figurePtr->material == Material::DIFFUSE) {
        Vec3 d = distribution.sample(u01, n01, rng, x + eps * norma, norma);
        if (d.dot(norma) < 0) {
            return figurePtr->emission;
        }
        float pdf = distribution.pdf(x + eps * norma, norma, d);
        Ray dRay = Ray(x + eps * d, d);
        return figurePtr->emission + 1. / (PI * pdf) * d.dot(norma) * figurePtr->color * getColor(u01, n01, rng, dRay, recLimit - 1);
    } else if (figurePtr->material == Material::METALLIC) {
        Vec3 reflectedDir = ray.d.normalize() - 2. * norma.dot(ray.d.normalize()) * norma;
        Ray reflected = Ray(ray.o + t * ray.d + eps * reflectedDir, reflectedDir);
        return figurePtr->emission + figurePtr->color * getColor(u01, n01, rng, reflected, recLimit - 1);
    } else {
        Vec3 reflectedDir = ray.d.normalize() - 2. * norma.dot(ray.d.normalize()) * norma;
        Ray reflected = Ray(ray.o + t * ray.d + eps * reflectedDir, reflectedDir);
        Color reflectedColor = getColor(u01, n01, rng, reflected, recLimit - 1);

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
        Ray refracted = Ray(ray.o + t * ray.d + eps * refractedDir, refractedDir);
        Color refractedColor = getColor(u01, n01, rng, refracted, recLimit - 1);
        if (!is_inside) {
            refractedColor = refractedColor * figurePtr->color;
        }
        return figurePtr->emission + refractedColor;
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
    float tanFovX = tan(cameraFovX / 2);
    float tanFovY = tanFovX * height / width;

    float nx = tanFovX * (2 * x / width - 1);
    float ny = tanFovY * (2 * y / height - 1);

    return {cameraPos, nx * cameraRight - ny * cameraUp + cameraForward};
}