#include "scene.h"
#include <cmath>
#include <random>

static std::minstd_rand rng;
static std::uniform_real_distribution<float> u01(0.0, 1.0);
static Cosine main_dist = Cosine(rng);

Scene::Scene() {}

void Scene::initDistribution() {
    std::vector<std::unique_ptr<Distribution>> lightDistributions;
    for (const auto &figure: figures) {
        if (figure.emission.x > 0 || figure.emission.y > 0 || figure.emission.z > 0) {
            if (figure.type == FigureType::BOX) {
                lightDistributions.push_back(std::unique_ptr<Distribution>(new BoxLight(rng, figure)));
            } else if (figure.type == FigureType::ELLIPSOID) {
                lightDistributions.push_back(std::unique_ptr<Distribution>(new EllipsoidLight(rng, figure)));
            }
        }
    }
    std::vector<std::unique_ptr<Distribution>> finalDistributions;
    finalDistributions.push_back(std::unique_ptr<Distribution>(new Cosine(rng)));
    if (!lightDistributions.empty()) {
        auto lightDistribution = std::unique_ptr<Distribution>(new Mix(rng, std::move(lightDistributions)));
        finalDistributions.push_back(std::move(lightDistribution));
    }
    distribution = std::unique_ptr<Distribution>(new Mix(rng, std::move(finalDistributions)));
}

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
    auto x = ray.o + t * ray.d;

    if (figurePtr->material == Material::DIFFUSE) {
        Vec3 d = distribution->sample(x + 0.0001 * norma, norma);
        float pdf = distribution->pdf(x + 0.0001 * norma, norma, d);
        Ray dRay = Ray(x + 0.0001 * d, d);
        if (d.dot(norma) < 0) {
            return figurePtr->emission;
        }
        return figurePtr->emission + 1. / (PI * pdf) * d.dot(norma) * figurePtr->color * getColor(dRay, recLimit - 1);
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
        if (u01(rng) < r) {
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
        float nx = x + u01(rng);
        float ny = y + u01(rng);
        color = color + getColor(getCameraRay(nx, ny), rayDepth);
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