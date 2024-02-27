#pragma once
#include "vec3.h"
#include <istream>

class Quaternion {
private:
    Vec3 v;
    float w;

public:
    Quaternion();
    Quaternion(float x, float y, float z, float w);
    Quaternion(Vec3 v, float w);

    Quaternion operator + (const Quaternion &other) const;
    Quaternion operator * (const Quaternion &other) const;
    Quaternion conjugate() const;
    Vec3 transform(const Vec3 &p) const;

    friend std::istream &operator >> (std::istream &in, Quaternion &q);
};

std::istream &operator >> (std::istream &in, Quaternion &q);