#pragma once
#include <optional>
#include <cassert>
#include "vec3.h"
#include "color.h"
#include "quaternion.h"
#include "gltf_structs.h"

const long double eps = 1e-4;

class Ray {
public:
    const Vec3 o, d;

    Ray();
    Ray(Vec3 o, Vec3 d);

    Ray operator + (const Vec3 &other) const;
    Ray operator - (const Vec3 &other) const;
    Ray rotate(const Quaternion &rotation) const;
};

struct Intersection {
    float t;
    Vec3 norma;
    bool is_inside;    
};

enum class FigureType {
    ELLIPSOID, PLANE, BOX, TRIANGLE
};

class Figure {
private:
    std::optional<Intersection> intersectAsEllipsoid(const Ray &ray) const;
    std::optional<Intersection> intersectAsPlane(const Ray &ray) const;
    std::optional<Intersection> intersectAsBox(const Ray &ray) const;
    std::optional<Intersection> intersectAsTriangle(const Ray &ray) const;

public:
    Vec3 position;
    Quaternion rotation;
    GltfMaterial material;

    FigureType type;
    Vec3 data;
    Vec3 data2;
    Vec3 data3;

    Figure();
    Figure(FigureType type, Vec3 data);
    Figure(FigureType type, Vec3 data, Vec3 data2, Vec3 data3);

    std::optional<Intersection> intersect(const Ray &ray) const;
};

class AABB {
public:
    Vec3 min;
    Vec3 max;

    AABB();
    AABB(const Vec3 &min, const Vec3 &max);
    AABB(const Figure &fig);

    void extend(const Vec3 &p);
    void extend(const AABB &aabb);
    float getS() const;

    std::optional<Intersection> intersect(const Ray &ray) const;
};


inline Ray::Ray() {}
inline Ray::Ray(Vec3 o, Vec3 d): o(o), d(d) {}

inline Ray Ray::operator + (const Vec3 &other) const {
    return {o + other, d};
}

inline Ray Ray::operator - (const Vec3 &other) const {
    return {o - other, d};
}

inline Ray Ray::rotate(const Quaternion &rotation) const {
    return {rotation.transform(o), rotation.transform(d)};
}