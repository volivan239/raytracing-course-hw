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

class Figure {
private:
    virtual std::optional<float> rawIntersect(const Ray &ray) const = 0;

public:
    Vec3 position = {0, 0, 0};
    Quaternion rotation = {0, 0, 0, 1};
    Material material = Material::DIFFUSE;
    Color color;

    Figure();

    std::optional<std::pair<float, Color>> intersect(const Ray &ray);
};

class Ellipsoid : public Figure {
private:
    std::optional<float> rawIntersect(const Ray &ray) const override;

public:
    Vec3 r;

    Ellipsoid();
    Ellipsoid(Vec3 r);
};

class Plane : public Figure {
private:
    std::optional<float> rawIntersect(const Ray &ray) const override;

public:
    Vec3 n;

    Plane();
    Plane(Vec3 n);
};

class Box : public Figure {
private:
    std::optional<float> rawIntersect(const Ray &ray) const override;

public:
    Vec3 s;

    Box();
    Box(Vec3 s);
};