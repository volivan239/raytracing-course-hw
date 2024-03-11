#pragma once
#include <inttypes.h>
#include <algorithm>
#include "vec3.h"

using Color = Vec3;

uint8_t* toExternColorFormat(const Color &color);

Color gamma_corrected(const Color &x);
Color aces_tonemap(const Color &x);