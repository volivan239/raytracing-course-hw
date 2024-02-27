#include "primitives.h"
#include <math.h>
#include <cassert>

Figure::Figure() {};

Figure::Figure(FigureType type, Vec3 data): type(type), data(data) {};

std::optional<Intersection> Figure::intersect(const Ray &ray) const {
    Ray transformed = (ray - position).rotate(rotation);

    std::optional<Intersection> result;
    if (type == FigureType::ELLIPSOID) {
        result = intersectAsEllipsoid(transformed);
    } else if (type == FigureType::PLANE) {
        result = intersectAsPlane(transformed);
    } else {
        result = intersectAsBox(transformed);
    }

    if (!result.has_value()) {
        return {};
    }
    auto [t, norma, is_inside] = result.value();
    norma = rotation.conjugate().transform(norma).normalize();
    return {Intersection {t, norma, is_inside}};
}

std::optional<std::pair<float, bool>> smallestPositiveRootOfSquareEquation(float a, float b, float c) {
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
        return {std::make_pair(x2, true)};
    } else {
        return {std::make_pair(x1, false)};
    }
}

std::optional<Intersection> Figure::intersectAsEllipsoid(const Ray &ray) const {
    Vec3 r = data;
    float c = (ray.o / r).len2() - 1;
    float b = 2. * (ray.o / r).dot(ray.d / r);
    float a = (ray.d / r).len2();

    auto opt_t = smallestPositiveRootOfSquareEquation(a, b, c);
    if (!opt_t.has_value()) {
        return {};
    }

    auto [t, is_inside] = opt_t.value();
    Vec3 point = ray.o + t * ray.d;
    Vec3 norma = point / (r * r);
    if (is_inside) {
        norma = -1. * norma;
    }
    assert(norma.dot(ray.d) <= 0);
    return {Intersection {t, norma.normalize(), is_inside}};
}

std::optional<Intersection> Figure::intersectAsPlane(const Ray &ray) const {
    Vec3 n = data;
    float t = -ray.o.dot(n) / ray.d.dot(n);
    if (t > 0) {
        if (ray.d.dot(n) > 0) {
            return {Intersection {t, -1. * n, true}};
        }
        return {Intersection {t, n, false}};
    }
    return {};
}

std::optional<Intersection> Figure::intersectAsBox(const Ray &ray) const {
    Vec3 s = data;
    Vec3 ts1 = (-1. * s - ray.o) / ray.d;
    Vec3 ts2 = (s - ray.o) / ray.d;
    float t1x = std::min(ts1.x, ts2.x), t2x = std::max(ts1.x, ts2.x);
    float t1y = std::min(ts1.y, ts2.y), t2y = std::max(ts1.y, ts2.y);
    float t1z = std::min(ts1.z, ts2.z), t2z = std::max(ts1.z, ts2.z);
    float t1 = std::max(std::max(t1x, t1y), t1z);
    float t2 = std::min(std::min(t2x, t2y), t2z);
    if (t1 > t2 || t2 < 0) {
        return {};
    }
    
    float t;
    bool is_inside;
    if (t1 < 0) {
        is_inside = true;
        t = t2;
    } else {
        is_inside = false;
        t = t1;
    }

    Vec3 p = ray.o + t * ray.d;
    Vec3 norma = p / s;
    float mx = std::max(std::max(fabs(norma.x), fabs(norma.y)), fabs(norma.z));

    if (fabs(norma.x) != mx) {
        norma.x = 0;
    }
    if (fabs(norma.y) != mx) {
        norma.y = 0;
    }
    if (fabs(norma.z) != mx) {
        norma.z = 0;
    }

    if (is_inside) {
        norma = -1. * norma;
    }

    assert(ray.d.dot(norma) <= 0);
    return {Intersection {t, norma, is_inside}};
}