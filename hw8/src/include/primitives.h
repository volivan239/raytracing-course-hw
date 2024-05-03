#pragma once
#include <optional>
#include <cassert>
#include "vec3.h"
#include "color.h"
#include "quaternion.h"
#include "gltf_structs.h"

const long double eps = 1e-4;

struct Vec2 {
    float x, y;
    Vec2() {}
    Vec2(float x, float y): x(x), y(y) {}
};

struct Vec4 {
    Vec3 v;
    float w;

    Vec4() {}
    Vec4(float x, float y, float z, float w): v(x, y, z), w(w) {}
    Vec4(Vec3 v, float w): v(v), w(w) {}
};

class Vertex {
public:
    Vec3 coords;
    Vec2 texcoords;
    Vec3 normals;
    Vec4 tangents;

    Vertex() {}
    Vertex(Vec3 coords, Vec2 texcoords, Vec3 normals, Vec4 tangents): coords(coords), texcoords(texcoords), normals(normals), tangents(tangents) {}
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
    std::optional<Vec2> texcoords;
    std::optional<Vec3> shadingNorma;
    std::optional<Vec4> tangent;
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