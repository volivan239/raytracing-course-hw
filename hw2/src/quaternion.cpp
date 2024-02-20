#include "quaternion.h"
#include <math.h>

Quaternion::Quaternion(): v{0, 0, 0}, w(0) {}

Quaternion::Quaternion(float x, float y, float z, float w): v{x, y, z}, w(w) {}

Quaternion::Quaternion(Vec3 v, float w): v(v), w(w) {}

Quaternion Quaternion::operator + (const Quaternion &other) const {
    return {v + other.v, w + other.w};
}

Quaternion Quaternion::operator * (const Quaternion &other) const {
    return {w * other.v + other.w * v + v.cross(other.v), w * other.w - v.dot(other.v)};
}

Quaternion Quaternion::conjugate() const {
    return {(-1.) * v, w};
}

Vec3 Quaternion::transform(const Vec3 &p) const {
    return ((*this) * Quaternion(p.x, p.y, p.z, 0.) * conjugate()).v;
}

std::istream &operator >> (std::istream &in, Quaternion &q) {
    in >> q.v >> q.w;
    return in;
}