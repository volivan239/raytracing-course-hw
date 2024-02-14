#pragma once
#include "point.h"
#include <istream>

class Quaternion {
private:
    Point v;
    float w;

public:
    Quaternion();
    Quaternion(float x, float y, float z, float w);
    Quaternion(Point v, float w);

    Quaternion operator + (const Quaternion &other) const;
    Quaternion operator * (const Quaternion &other) const;
    Quaternion conjugate() const;
    Point transform(const Point &p) const;

    friend std::istream &operator >> (std::istream &in, Quaternion &q);
};

std::istream &operator >> (std::istream &in, Quaternion &q);