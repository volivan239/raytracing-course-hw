#include "color.h"
#include <cassert>
#include <math.h>

uint8_t* toExternColorFormat(const Color &color) {
    uint8_t *ans = new uint8_t[3];
    ans[0] = round(255 * color.x);
    ans[1] = round(255 * color.y);
    ans[2] = round(255 * color.z);
    return ans;
}

Color saturate(const Color &color) {
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