#include "color.h"
#include <cmath>

std::array<uint8_t, 3> toExternColorFormat(const Color &color) {
    uint8_t ans0 = round(255 * color.x);
    uint8_t ans1 = round(255 * color.y);
    uint8_t ans2 = round(255 * color.z);
    return {ans0, ans1, ans2};
}

Color gamma_corrected(const Color &x) {
    float gamma = 1. / 2.2;
    return Color(pow(x.x, gamma), pow(x.y, gamma), pow(x.z, gamma));
}


static Color saturate(const Color &color) {
    float x = std::min(1.f, std::max(0.f, color.x));
    float y = std::min(1.f, std::max(0.f, color.y));
    float z = std::min(1.f, std::max(0.f, color.z));
    return Color(x, y, z);
}

Color aces_tonemap(const Color &x) {
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return saturate((x*(a*x+b))/(x*(c*x+d)+e));
}