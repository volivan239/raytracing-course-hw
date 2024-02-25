#include "vec3.h"
#include <math.h>

Vec3::Vec3() {}

Vec3::Vec3(float x, float y, float z): x(x), y(y), z(z) {}

Vec3 Vec3::operator + (const Vec3 &other) const {
    return {x + other.x, y + other.y, z + other.z};
}

Vec3 Vec3::operator - (const Vec3 &other) const {
    return {x - other.x, y - other.y, z - other.z};
}

Vec3 Vec3::operator / (const Vec3 &other) const {
    return {x / other.x, y / other.y, z / other.z};
}

Vec3 Vec3::operator * (const Vec3 &other) const {
    return {x * other.x, y * other.y, z * other.z};
}

Vec3 Vec3::operator + (float v) const {
    return {x + v, y + v, z + v};
}

float Vec3::dot(const Vec3 &other) const {
    return x * other.x + y * other.y + z * other.z;
}

Vec3 Vec3::cross(const Vec3 &other) const {
    return {z * other.y - y * other.z, x * other.z - z * other.x, y * other.x - x * other.y};
}

float Vec3::len2() const {
    return dot(*this);
}

float Vec3::len() const {
    return sqrt(len2());
}

std::istream& operator >> (std::istream &in, Vec3 &point) {
    in >> point.x >> point.y >> point.z;
    return in;
}

Vec3 operator * (float k, const Vec3 &p) {
    return {k * p.x, k * p.y, k * p.z};
}

Vec3 Vec3::normalize() const {
    return 1. / len() * (*this);
}