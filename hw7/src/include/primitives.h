#pragma once
#include <optional>
#include <cassert>
#include "vec3.h"
#include "color.h"
#include "quaternion.h"
#include "gltf_structs.h"

const long double eps = 1e-4;

class Vertex {
public:
    Vec3 coords;
    Vec3 normals;

    Vertex() {}
    Vertex(Vec3 coords, Vec3 normals): coords(coords), normals(normals) {}
};

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
    Vec3 geomNorma;
    std::optional<Vec3> shadingNorma;
    bool is_inside;    
};

class Figure {
private:
    std::optional<Intersection> intersectAsTriangle(const Ray &ray) const;

public:
    size_t materialIndex;
    GltfMaterial material;

    Vertex data;
    Vertex data2;
    Vertex data3;

    Figure();
    Figure(Vertex data);
    Figure(Vertex data, Vertex data2, Vertex data3);

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