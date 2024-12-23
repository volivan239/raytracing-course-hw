#pragma once
#include <inttypes.h>
#include <algorithm>
#include "vec3.h"

using Color = Vec3;

std::array<uint8_t, 3> toExternColorFormat(const Color &color);

Color gamma_corrected(const Color &x);
Color aces_tonemap(const Color &x);