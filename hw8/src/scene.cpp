#include "scene.h"
#include "stb_image.h"
#include <cmath>
#include <random>
#include <algorithm>

static const size_t DIRTY_DENOISE_HACK_BUBEN = 6;

static Vec3 loadSingleFromTexture(int ix, int iy, const Texture &texture, bool isSRGB) {
    size_t offset = 3 * (ix + texture.width * iy);
    Vec3 res = (1. / 255) * Vec3{1.f * texture.data[offset], 1.f * texture.data[offset + 1], 1.f * texture.data[offset + 2]};
    if (isSRGB) {
        return Vec3{std::pow(res.x, 2.2f), std::pow(res.y, 2.2f), std::pow(res.z, 2.2f)};
    }
    return res;
}

static Vec3 sampleTexture(float texcoordX, float texcoordY, const Texture &texture, bool isSRGB) {
    texcoordX -= std::floor(texcoordX);
    texcoordY -= std::floor(texcoordY);

    texcoordX *= texture.width;
    texcoordY *= texture.height;

    int ix1 = std::floor(texcoordX), ix2 = (ix1 + 1) % texture.width;
    int iy1 = std::floor(texcoordY), iy2 = (iy1 + 1) % texture.height;

    float dx = texcoordX - ix1;
    float dy = texcoordY - iy1;
    Vec3 px1y1 = loadSingleFromTexture(ix1, iy1, texture, isSRGB);
    Vec3 px1y2 = loadSingleFromTexture(ix1, iy2, texture, isSRGB);
    Vec3 px2y1 = loadSingleFromTexture(ix2, iy1, texture, isSRGB);
    Vec3 px2y2 = loadSingleFromTexture(ix2, iy2, texture, isSRGB);
    return (1 - dx) * ((1 - dy) * px1y1 + dy * px1y2) + dx * ((1 - dy) * px2y1 + dy * px2y2);
}

static Vec3 applyNormalMaps(Vec3 shadingNorma, Vec4 tangent, Vec3 sample, bool isInside) {
    if (isInside) {
        shadingNorma = -1. * shadingNorma;
    }
    Vec3 localX = tangent.v;
    Vec3 localZ = shadingNorma;
    Vec3 localY = tangent.w * localX.cross(localZ);
    Vec3 localNorma = 2.f * sample - Vec3{1., 1., 1.};
    Vec3 norma = localNorma.x * localX + localNorma.y * localY + localNorma.z * localZ;
    if (isInside) {
        norma = -1. * norma;
        shadingNorma = -1. * shadingNorma;
    }
    norma = norma.normalize();
    // if (rand() % 3000000 == 0) {
    //     std::cerr << localX.dot(localZ) << ' ' << localY.dot(localZ) << ' ' << localX.dot(localY) << std::endl;
    //     std::cerr << localNorma.x << ' ' << localNorma.y << ' ' << localNorma.z << std::endl;
    //     std::cerr << norma.dot(shadingNorma) << std::endl;
    // }
    return norma.normalize();
}

Scene::Scene() {}

Scene::~Scene() {
    if (environmentMap.has_value()) {
        stbi_image_free(environmentMap.value().data);
    }
    for (const auto &texture : textureImages) {
        stbi_image_free(texture.data);
    }
}

void Scene::initDistribution() {
    auto lightDistribution = FiguresMix(figures);
    std::vector<std::variant<Cosine, Vndf, FiguresMix>> finalDistributions;
    finalDistributions.push_back(Cosine());
    finalDistributions.push_back(Vndf());
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
        if (!environmentMap.has_value()) {
            return bgColor;
        }
        float texcoordX = 0.5 + 0.5 * std::atan2(ray.d.z, ray.d.x) / M_PI;
        float texcoordY = 0.5 - std::asin(ray.d.y) / M_PI;
        return sampleTexture(texcoordX, texcoordY, environmentMap.value(), true);
    }

    auto [intersection, figurePos] = intersection_.value();
    auto [t, geomNorma, texcoords, shadingNorma_, tangent, isInside] = intersection;
    auto shadingNorma = shadingNorma_.value();
    auto figurePtr = figures.begin() + figurePos;
    const auto &material = figurePtr->material;
    auto x = ray.o + t * ray.d;

    const auto &materialModel = materialModels[figurePtr->materialIndex];
    Vec3 color{1, 1, 1};
    if (material.baseColorTexture.has_value()) {
        color = sampleTexture(
            texcoords.value().x,
            texcoords.value().y,
            textureImages[textureDescs[material.baseColorTexture.value()].source],
            true
        );
    }

    Vec3 emission = material.emission;
    if (material.emissiveTexture.has_value()) {
        emission = emission * sampleTexture(
            texcoords.value().x,
            texcoords.value().y,
            textureImages[textureDescs[material.emissiveTexture.value()].source],
            true
        );
    }

    Vec3 metallicRoughness = {1, 1, 1};
    if (material.metallicRoughnessTexture.has_value()) {
        metallicRoughness = sampleTexture(
            texcoords.value().x,
            texcoords.value().y,
            textureImages[textureDescs[material.metallicRoughnessTexture.value()].source],
            false
        );
    }

    if (material.normalTexture.has_value()) {
        Vec3 sample = sampleTexture(
            texcoords.value().x,
            texcoords.value().y,
            textureImages[textureDescs[material.normalTexture.value()].source],
            false
        );
        shadingNorma = applyNormalMaps(shadingNorma, tangent.value(), sample, isInside);
    }

    float alpha = pow(std::max(0.05f, material.roughnessFactor * metallicRoughness.y), 2.0);
    float metallic = metallicRoughness.z;

    Vec3 d = distribution.sample(u01, n01, rng, x + eps * geomNorma, shadingNorma, ray.d, alpha);
    Ray dRay = Ray(x + eps * geomNorma, d);
    Vec3 brdf = materialModel.brdf(dRay.d, -1. * ray.d, shadingNorma, color, metallic, alpha);
    if ((brdf.x < eps && brdf.y < eps && brdf.z < eps)) {
        return emission;
    }

    float pdf = distribution.pdf(x + eps * geomNorma, shadingNorma, d, ray.d, alpha);
    auto mult = 1. / pdf * fabs(d.dot(shadingNorma)) * brdf;

    if (mult.x > DIRTY_DENOISE_HACK_BUBEN || mult.y > DIRTY_DENOISE_HACK_BUBEN || mult.z > DIRTY_DENOISE_HACK_BUBEN || std::isnan(mult.x) || std::isnan(mult.y) || std::isnan(mult.z)) {
        return emission;
    }
    return emission + mult * getColor(u01, n01, rng, dRay, recLimit - 1);
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