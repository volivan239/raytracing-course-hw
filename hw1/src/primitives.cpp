#include "primitives.h"
#include <math.h>
#include <iostream>

Ray::Ray() {}
Ray::Ray(Point o, Point d): o(o), d(d) {}

Ray Ray::operator + (const Point &other) const {
    return {o + other, d};
}

Ray Ray::operator - (const Point &other) const {
    return {o - other, d};
}

Ray Ray::rotate(const Quaternion &rotation) const {
    return {rotation.transform(o), rotation.transform(d)};
}

Figure::Figure() {}

Ellipsoid::Ellipsoid() {}
Ellipsoid::Ellipsoid(Point r): r(r) {}

std::optional<std::pair<float, Color>> Figure::intersect(const Ray &ray) {
    Ray transformed = (ray - position).rotate(rotation);
    auto result = rawIntersect(transformed);
    if (result.has_value()) {
        return std::make_pair(result.value(), color);
    }
    return {};
}

std::optional<float> smallestPositiveRootOfSquareEquation(float a, float b, float c) {
    float d = b * b - 4 * a * c;
    if (d <= 0) {
        return {};
    }

    float x1 = (-b - sqrt(d)) / (2 * a);
    float x2 = (-b + sqrt(d)) / (2 * a);
    if (x1 > x2) {
        std::swap(x1, x2);
    }

    if (x2 < 0) {
        return {};
    } else if (x1 < 0) {
        return x2;
    } else {
        return x1;
    }
}

std::optional<float> Ellipsoid::rawIntersect(const Ray &ray) const {
    float c = (ray.o / r).len2() - 1;
    float b = 2 * (ray.o / r) * (ray.d / r);
    float a = (ray.d / r).len2();
    return smallestPositiveRootOfSquareEquation(a, b, c);
}

Plane::Plane() {}
Plane::Plane(Point n): n(n) {}

std::optional<float> Plane::rawIntersect(const Ray &ray) const {
    float t = -(ray.o * n) / (ray.d * n);
    if (t > 0) {
        return t;
    }
    return {};
}


Box::Box() {}
Box::Box(Point s): s(s) {}

std::optional<float> Box::rawIntersect(const Ray &ray) const {
    Point ts1 = (-1. * s - ray.o) / ray.d;
    Point ts2 = (s - ray.o) / ray.d;
    float t1x = std::min(ts1.x, ts2.x), t2x = std::max(ts1.x, ts2.x);
    float t1y = std::min(ts1.y, ts2.y), t2y = std::max(ts1.y, ts2.y);
    float t1z = std::min(ts1.z, ts2.z), t2z = std::max(ts1.z, ts2.z);
    float t1 = std::max(std::max(t1x, t1y), t1z);
    float t2 = std::min(std::min(t2x, t2y), t2z);
    if (t1 > t2 || t2 < 0) {
        return {};
    } else if (t1 < 0) {
        return t2;
    } else {
        return t1;
    }
}