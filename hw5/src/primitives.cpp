#include "primitives.h"
#include <cmath>
#include <cassert>

Figure::Figure() {};

Figure::Figure(FigureType type, Vec3 data): type(type), data(data) {};

Figure::Figure(FigureType type, Vec3 data, Vec3 data2, Vec3 data3): type(type), data(data), data2(data2), data3(data3) {};

static const float T_MAX = 1e4;

std::optional<Intersection> Figure::intersect(const Ray &ray) const {
    Ray transformed = (ray - position).rotate(rotation);

    std::optional<Intersection> result;
    if (type == FigureType::ELLIPSOID) {
        result = intersectAsEllipsoid(transformed);
    } else if (type == FigureType::PLANE) {
        result = intersectAsPlane(transformed);
    } else if (type == FigureType::BOX) {
        result = intersectAsBox(transformed);
    } else {
        result = intersectAsTriangle(transformed);
    }

    if (!result.has_value()) {
        return {};
    }
    auto [t, norma, is_inside] = result.value();
    norma = rotation.conjugate().transform(norma).normalize();
    return {Intersection {t, norma, is_inside}};
}

std::optional<std::pair<float, bool>> smallestPositiveRootOfQuadraticEquation(float a, float b, float c) {
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

    auto opt_t = smallestPositiveRootOfQuadraticEquation(a, b, c);
    if (!opt_t.has_value()) {
        return {};
    }

    auto [t, is_inside] = opt_t.value();
    Vec3 point = ray.o + t * ray.d;
    Vec3 norma = point / (r * r);
    if (is_inside) {
        norma = -1. * norma;
    }
    assert(true);
    return {Intersection {t, norma.normalize(), is_inside}};
}

std::optional<Intersection> intersectPlaneAndRay(const Vec3 &n, const Ray &ray) {
    float t = -ray.o.dot(n) / ray.d.dot(n);
    if (t > 0 && t < T_MAX) {
        if (ray.d.dot(n) > 0) {
            return {Intersection {t, -1. * n, true}};
        }
        return {Intersection {t, n, false}};
    }
    return {};
}

std::optional<Intersection> Figure::intersectAsPlane(const Ray &ray) const {
    return intersectPlaneAndRay(data, ray);
}

std::optional<Intersection> intersectBoxAndRay(const Vec3 &s, const Ray &ray) {
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

    return {Intersection {t, norma, is_inside}};
}

std::optional<Intersection> Figure::intersectAsBox(const Ray &ray) const {
    return intersectBoxAndRay(data, ray);
}   

std::optional<Intersection> Figure::intersectAsTriangle(const Ray &ray) const {
    const Vec3 &a = data3;
    const Vec3 &b = data - a;
    const Vec3 &c = data2 - a;
    Vec3 n = b.cross(c);
    auto intersection = intersectPlaneAndRay(n, ray - a);
    if (!intersection.has_value()) {
        return {};
    }
    auto t = intersection.value().t;
    Vec3 p = ray.o - a + t * ray.d;
    if (b.cross(p).dot(n) < 0) {
        return {};
    }
    if (p.cross(c).dot(n) < 0) {
        return {};
    }
    if ((c - b).cross(p - b).dot(n) < 0) {
        return {};
    }
    return intersection;
}

AABB::AABB() {}
AABB::AABB(const Vec3 &min, const Vec3 &max): min(min), max(max) {}

AABB::AABB(const Figure &fig) {
    if (fig.type == FigureType::BOX || fig.type == FigureType::ELLIPSOID) {
        min = (-1.) * fig.data;
        max = fig.data;
    } else if (fig.type == FigureType::TRIANGLE) {
        min = Vec3(
            std::min(fig.data3.x, std::min(fig.data.x, fig.data2.x)),
            std::min(fig.data3.y, std::min(fig.data.y, fig.data2.y)),
            std::min(fig.data3.z, std::min(fig.data.z, fig.data2.z))
        );
        max = Vec3(
            std::max(fig.data3.x, std::max(fig.data.x, fig.data2.x)),
            std::max(fig.data3.y, std::max(fig.data.y, fig.data2.y)),
            std::max(fig.data3.z, std::max(fig.data.z, fig.data2.z))
        );
    } else {
        assert(false);
    }
    auto rotation = fig.rotation.conjugate();
    AABB unbiased = *this;
    min = max = rotation.transform(unbiased.min);
    extend(rotation.transform(Vec3(unbiased.min.x, unbiased.min.y, unbiased.max.z)));
    extend(rotation.transform(Vec3(unbiased.min.x, unbiased.max.y, unbiased.min.z)));
    extend(rotation.transform(Vec3(unbiased.min.x, unbiased.max.y, unbiased.max.z)));
    extend(rotation.transform(Vec3(unbiased.max.x, unbiased.min.y, unbiased.min.z)));
    extend(rotation.transform(Vec3(unbiased.max.x, unbiased.min.y, unbiased.max.z)));
    extend(rotation.transform(Vec3(unbiased.max.x, unbiased.max.y, unbiased.min.z)));
    extend(rotation.transform(Vec3(unbiased.max.x, unbiased.max.y, unbiased.max.z)));
    min = min + fig.position;
    max = max + fig.position;
}


void AABB::extend(const Vec3 &p) {
    max.x = std::max(max.x, p.x);
    max.y = std::max(max.y, p.y);
    max.z = std::max(max.z, p.z);
    min.x = std::min(min.x, p.x);
    min.y = std::min(min.y, p.y);
    min.z = std::min(min.z, p.z);
}

void AABB::extend(const AABB &aabb) {
    extend(aabb.min);
    extend(aabb.max);
}

float AABB::getS() const {
    Vec3 d = max - min;
    return 2 * (d.x * d.y + d.x * d.z + d.y * d.z);
}

std::optional<Intersection> AABB::intersect(const Ray &ray) const {
    return intersectBoxAndRay(0.5 * (max - min), ray - 0.5 * (min + max));
}