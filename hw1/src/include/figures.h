#pragma once
#include <optional>
#include "point.h"
#include "color.h"
#include "quaternion.h"

class Ray {
public:
    const Point o, d;

    Ray();
    Ray(Point o, Point d);

    Ray operator + (const Point &other) const;
    Ray operator - (const Point &other) const;
    Ray rotate(const Quaternion &rotation) const;
};

class Figure {
private:
    virtual std::optional<float> rawIntersect(const Ray &ray) const = 0;

public:
    Point position = {0, 0, 0};
    Quaternion rotation = {0, 0, 0, 1};
    Color color;

    Figure();

    std::optional<std::pair<float, Color>> intersect(const Ray &ray);
};

class Ellipsoid : public Figure {
private:
    std::optional<float> rawIntersect(const Ray &ray) const override;

public:
    Point r;

    Ellipsoid();
    Ellipsoid(Point r);
};

class Plane : public Figure {
private:
    std::optional<float> rawIntersect(const Ray &ray) const override;

public:
    Point n;

    Plane();
    Plane(Point n);
};