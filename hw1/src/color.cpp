#include "color.h"
#include <cassert>
#include <math.h>

Color::Color() {}

Color::Color(float r, float g, float b): r(r), g(g), b(b) {
    assert(0 <= r && r <= 1);
    assert(0 <= g && g <= 1);
    assert(0 <= b && b <= 1);
}

uint8_t* Color::toExternFormat() const {
    uint8_t *ans = new uint8_t[3];
    ans[0] = round(255 * r);
    ans[1] = round(255 * g);
    ans[2] = round(255 * b);
    return ans;
}

std::istream& operator >> (std::istream &in, Color &color) {
    in >> color.r >> color.g >> color.b;
    return in;
}
