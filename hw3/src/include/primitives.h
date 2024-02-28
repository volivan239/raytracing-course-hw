#pragma once
#include <optional>
#include "vec3.h"
#include "color.h"
#include "quaternion.h"
#include "material.h"

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
    ELLIPSOID, PLANE, BOX
};

class Figure {
private:
    std::optional<Intersection> intersectAsEllipsoid(const Ray &ray) const;
    std::optional<Intersection> intersectAsPlane(const Ray &ray) const;
    std::optional<Intersection> intersectAsBox(const Ray &ray) const;

public:
    Vec3 position = {0, 0, 0};
    Quaternion rotation = {0, 0, 0, 1};
    Material material = Material::DIFFUSE;
    Color color;
    Color emission = {0, 0, 0};
    float ior;

    FigureType type;
    Vec3 data;

    Figure();
    Figure(FigureType type, Vec3 data);

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