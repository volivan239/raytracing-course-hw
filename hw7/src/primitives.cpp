#include "primitives.h"
#include <cmath>
#include <cassert>
#include <iostream>

Figure::Figure() {};

Figure::Figure(Vertex data): data(data) {};

Figure::Figure(Vertex data, Vertex data2, Vertex data3): data(data), data2(data2), data3(data3) {};

static const float T_MAX = 1e4;

std::optional<Intersection> Figure::intersect(const Ray &ray) const {
    Ray transformed = (ray - position).rotate(rotation);

    std::optional<Intersection> result = intersectAsTriangle(transformed);

    if (!result.has_value()) {
        return {};
    }
    auto [t, geomNorma, shadingNorma, is_inside] = result.value();
    geomNorma = rotation.conjugate().transform(geomNorma).normalize();
    shadingNorma = rotation.conjugate().transform(shadingNorma.value()).normalize();
    return {Intersection {t, geomNorma, shadingNorma, is_inside}};
}

std::optional<Intersection> intersectPlaneAndRay(const Vec3 &n, const Ray &ray) {
    float t = -ray.o.dot(n) / ray.d.dot(n);
    if (t > 0 && t < T_MAX) {
        if (ray.d.dot(n) > 0) {
            return {Intersection {t, -1. * n, {}, true}};
        }
        return {Intersection {t, n, {}, false}};
    }
    return {};
}

std::optional<Intersection> intersectBoxAndRay(const Vec3 &s, const Ray &ray, bool require_norma = true) {
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

    if (!require_norma) {
        return {Intersection {t, {}, {}, is_inside}};
    }

    Vec3 p = ray.o + t * ray.d;
    Vec3 geomNorma = p / s;
    float mx = std::max(std::max(fabs(geomNorma.x), fabs(geomNorma.y)), fabs(geomNorma.z));

    if (fabs(geomNorma.x) != mx) {
        geomNorma.x = 0;
    }
    if (fabs(geomNorma.y) != mx) {
        geomNorma.y = 0;
    }
    if (fabs(geomNorma.z) != mx) {
        geomNorma.z = 0;
    }

    if (is_inside) {
        geomNorma = -1. * geomNorma;
    }

    return {Intersection {t, geomNorma, {}, is_inside}};
}  

std::pair<float, float> solveLinearSystem(float a1, float b1, float c1, float a2, float b2, float c2) {
    float y = (c1 * a2 - c2 * a1) / (b1 * a2 - a1 * b2);
    float x = a2 == 0 ? (c1 - b1 * y) / a1 : (c2 - b2 * y) / a2;
    return {x, y};
}

static const float magic1[] = {0.239, 0.419, 0.533};
static const float magic2[] = {0.35743, 0.66682, 0.69695};

std::optional<Intersection> Figure::intersectAsTriangle(const Ray &ray) const {
    const Vec3 &a = data3.coords;
    const Vec3 &b = data.coords - a;
    const Vec3 &c = data2.coords - a;
    Vec3 n = b.cross(c);
    auto intersection = intersectPlaneAndRay(n, ray - a);
    if (!intersection.has_value()) {
        return {};
    }
    auto [t, geomNorma, _, is_inside] = intersection.value();
    Vec3 p = ray.o - a + t * ray.d;

    auto a1 = magic1[0] * b.x + magic1[1] * b.y + magic1[2] * b.z;
    auto b1 = magic1[0] * c.x + magic1[1] * c.y + magic1[2] * c.z;
    auto c1 = magic1[0] * p.x + magic1[1] * p.y + magic1[2] * p.z;

    auto a2 = magic2[0] * b.x + magic2[1] * b.y + magic2[2] * b.z;
    auto b2 = magic2[0] * c.x + magic2[1] * c.y + magic2[2] * c.z;
    auto c2 = magic2[0] * p.x + magic2[1] * p.y + magic2[2] * p.z;
    auto [u, v] = solveLinearSystem(a1, b1, c1, a2, b2, c2);

    if (u < 0 || v < 0 || u > 1 || v > 1 || u + v > 1) {
        return {};
    }

    Vec3 shadingNorma = data3.normals + u * (data.normals - data3.normals) + v * (data2.normals - data3.normals);
    shadingNorma = shadingNorma.normalize();
    if (is_inside) {
        shadingNorma = -1. * shadingNorma;
    }
    geomNorma = geomNorma.normalize();
    return {{t, geomNorma, shadingNorma, is_inside}};
}

AABB::AABB() {}
AABB::AABB(const Vec3 &min, const Vec3 &max): min(min), max(max) {}

AABB::AABB(const Figure &fig) {
    min = Vec3(
        std::min(fig.data3.coords.x, std::min(fig.data.coords.x, fig.data2.coords.x)),
        std::min(fig.data3.coords.y, std::min(fig.data.coords.y, fig.data2.coords.y)),
        std::min(fig.data3.coords.z, std::min(fig.data.coords.z, fig.data2.coords.z))
    );
    max = Vec3(
        std::max(fig.data3.coords.x, std::max(fig.data.coords.x, fig.data2.coords.x)),
        std::max(fig.data3.coords.y, std::max(fig.data.coords.y, fig.data2.coords.y)),
        std::max(fig.data3.coords.z, std::max(fig.data.coords.z, fig.data2.coords.z))
    );

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
    return intersectBoxAndRay(0.5 * (max - min), ray - 0.5 * (min + max), false);
}