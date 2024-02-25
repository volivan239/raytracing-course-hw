#include "scene.h"
#include <assert.h>
#include <math.h>

Scene::Scene() {}

std::optional<std::pair<float, Color>> Scene::intersect(const Ray &ray, float tmax, int recLimit) const {
    if (recLimit == 0) {
        return {};
    }

    float best_dist;
    int best_pos = -1;
    for (int i = 0; i < (int) figures.size(); i++) {
        auto intersection = figures[i]->intersect(ray);
        if (intersection.has_value()) {
            auto [t, _, __] = intersection.value();
            if (t <= tmax && (best_pos == -1 || t < best_dist)) {
                best_dist = t;
                best_pos = i;
            }
        }
    }

    if (best_pos == -1) {
        return {};
    }

    auto [t, norma, is_inside] = figures[best_pos]->intersect(ray).value();
    if (/*figures[best_pos]->material == Material::DIFFUSE*/ true) {
        Color color = ambientLight;
        for (const auto &lightSource : lightSources) {
            Vec3 p = ray.o + t * ray.d;
            auto [l, c, tmax] = lightSource->lightAt(p);
            float reflected = l.dot(norma);
            if (reflected >= 0 && !intersect(Ray(p + 0.0001 * l, l), tmax, recLimit - 1).has_value()) {
                color = color + reflected * c;
            }
        }
        return {std::make_pair(t, color * figures[best_pos]->color)};
    }
}

Color Scene::getPixel(int x, int y) const {
    auto ans = intersect(getCameraRay(x, y));
    if (ans.has_value()) {
        return ans.value().second;
    }
    
    return bgColor;
}

Ray Scene::getCameraRay(int x, int y) const {
    float tanFovX = tan(cameraFovX / 2);
    float tanFovY = tanFovX * height / width;

    float nx = tanFovX * (2 * (x + 0.5) / width - 1);
    float ny = tanFovY * (2 * (y + 0.5) / height - 1);

    return {cameraPos, nx * cameraRight - ny * cameraUp + cameraForward};
}