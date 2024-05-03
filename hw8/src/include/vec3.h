#pragma once
#include <istream>
#include <cmath>

class Vec3 {
public:
    float x = 0, y = 0, z = 0;

    Vec3();
    Vec3(float x, float y, float z);

    Vec3 operator + (const Vec3 &other) const;
    Vec3 operator - (const Vec3 &other) const;
    Vec3 operator / (const Vec3 &other) const;
    Vec3 operator * (const Vec3 &other) const;
    Vec3 operator + (float v) const;
    float dot(const Vec3 &other) const;
    Vec3 cross(const Vec3 &other) const;

    Vec3 normalize() const;
    float len() const;
    float len2() const;
};

std::istream& operator >> (std::istream &in, Vec3 &point);
Vec3 operator * (float k, const Vec3 &p);


inline Vec3::Vec3() {}

inline Vec3::Vec3(float x, float y, float z): x(x), y(y), z(z) {}

inline Vec3 Vec3::operator + (const Vec3 &other) const {
    return {x + other.x, y + other.y, z + other.z};
}

inline Vec3 Vec3::operator - (const Vec3 &other) const {
    return {x - other.x, y - other.y, z - other.z};
}

inline Vec3 Vec3::operator / (const Vec3 &other) const {
    return {x / other.x, y / other.y, z / other.z};
}

inline Vec3 Vec3::operator * (const Vec3 &other) const {
    return {x * other.x, y * other.y, z * other.z};
}

inline Vec3 Vec3::operator + (float v) const {
    return {x + v, y + v, z + v};
}

inline float Vec3::dot(const Vec3 &other) const {
    return x * other.x + y * other.y + z * other.z;
}

inline Vec3 Vec3::cross(const Vec3 &other) const {
    return {z * other.y - y * other.z, x * other.z - z * other.x, y * other.x - x * other.y};
}

inline float Vec3::len2() const {
    return dot(*this);
}

inline float Vec3::len() const {
    return sqrt(len2());
}

inline std::istream& operator >> (std::istream &in, Vec3 &point) {
    in >> point.x >> point.y >> point.z;
    return in;
}

inline Vec3 operator * (float k, const Vec3 &p) {
    return {k * p.x, k * p.y, k * p.z};
}

inline Vec3 Vec3::normalize() const {
    return 1. / len() * (*this);
}