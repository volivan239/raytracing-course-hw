#include "scene.h"
#include <assert.h>
#include <math.h>

Scene::Scene() {}

Color Scene::getPixel(int x, int y) const {
    Color ans = bgColor;
    float best_dist = -1;
    for (auto &figure : figures) {
        auto intersection = figure->intersect(getCameraRay(x, y));
        if (intersection.has_value()) {
            if (best_dist == -1 || intersection.value().first < best_dist) {
                best_dist = intersection.value().first;
                ans = intersection.value().second;
            }
        }
    }
    return ans;
}

Ray Scene::getCameraRay(int x, int y) const {
    float tanFovX = tan(cameraFovX / 2);
    float tanFovY = tanFovX * height / width;

    float nx = tanFovX * (2 * (x + 0.5) / width - 1);
    float ny = tanFovY * (2 * (y + 0.5) / height - 1);

    return {cameraPos, nx * cameraRight - ny * cameraUp + cameraForward};
}