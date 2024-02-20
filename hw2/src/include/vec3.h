#pragma once
#include <istream>

class Vec3 {
public:
    float x, y, z;

    Vec3();
    Vec3(float x, float y, float z);

    Vec3 operator + (const Vec3 &other) const;
    Vec3 operator - (const Vec3 &other) const;
    Vec3 operator / (const Vec3 &other) const;
    Vec3 operator * (const Vec3 &other) const;
    Vec3 operator + (float v) const;
    float dot(const Vec3 &other) const;
    Vec3 cross(const Vec3 &other) const;
    float len() const;
    float len2() const;
};

std::istream& operator >> (std::istream &in, Vec3 &point);
Vec3 operator * (float k, const Vec3 &p);