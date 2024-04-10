#pragma once
#include "vec3.h"
#include <istream>

class Quaternion {
public:
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


inline Quaternion::Quaternion(): v{0, 0, 0}, w(1) {}

inline Quaternion::Quaternion(float x, float y, float z, float w): v{x, y, z}, w(w) {}

inline Quaternion::Quaternion(Vec3 v, float w): v(v), w(w) {}

inline Quaternion Quaternion::operator + (const Quaternion &other) const {
    return {v + other.v, w + other.w};
}

inline Quaternion Quaternion::operator * (const Quaternion &other) const {
    return {w * other.v + other.w * v + v.cross(other.v), w * other.w - v.dot(other.v)};
}

inline Quaternion Quaternion::conjugate() const {
    return {(-1.) * v, w};
}

inline Vec3 Quaternion::transform(const Vec3 &p) const {
    return ((*this) * Quaternion(p.x, p.y, p.z, 0.) * conjugate()).v;
}

inline std::istream &operator >> (std::istream &in, Quaternion &q) {
    in >> q.v >> q.w;
    return in;
}