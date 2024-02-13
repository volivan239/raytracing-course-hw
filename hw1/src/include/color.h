#pragma once
#include <inttypes.h>
#include <istream>

class Color {
public:
    float r, g, b;
    Color();
    Color(float r, float g, float b);
    uint8_t* toExternFormat() const;
};

std::istream& operator >> (std::istream &in, Color &color);