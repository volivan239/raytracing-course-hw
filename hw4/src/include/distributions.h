#pragma once
#include <random>
#include <cmath>
#include <memory>
#include "vec3.h"
#include "primitives.h"

typedef std::minstd_rand rng_type;
const float PI = acos(-1);


class Distribution {
public:
    virtual Vec3 sample(Vec3 x, Vec3 n) = 0;
    virtual float pdf(Vec3 x, Vec3 n, Vec3 d) const = 0;
};


class Uniform : public Distribution {
private:
    std::normal_distribution<float> n01{0.f, 1.f};
    rng_type &rng;

public:
    Uniform(rng_type &rng): rng(rng) {}

    Vec3 sample(Vec3 x, Vec3 n) override {
        (void) x;
        Vec3 d = Vec3 {n01(rng), n01(rng), n01(rng)}.normalize();
        if (d.dot(n) < 0) {
            d = -1. * d;
        }
        return d;
    }

    float pdf(Vec3 x, Vec3 n, Vec3 d) const override {
        (void) x;
        if (d.dot(n) < 0) {
            return 0;
        }
        return 1. / (2. * PI);
    }
};


class Cosine : public Distribution {
private:
    std::normal_distribution<float> n01{0.f, 1.f};
    rng_type &rng;
    static constexpr float eps = 1e-9;

public:
    Cosine(rng_type &rng): rng(rng) {}

    Vec3 sample(Vec3 x, Vec3 n) override {
        (void) x;
        Vec3 d = Vec3 {n01(rng), n01(rng), n01(rng)}.normalize();
        d = d + n;
        float len = d.len();
        if (len <= eps || d.dot(n) <= eps || std::isnan(len)) {
            // Can happen due to precision errors
            return n;
        }
        Vec3 res = 1. / len * d;
        return res;
        return 1. / len * d;
    }

    float pdf(Vec3 x, Vec3 n, Vec3 d) const override {
        (void) x;
        return std::max(0.f, d.dot(n) / PI);
    }
};

class FigureLight : public Distribution {
private:
    virtual float pdfOne(Vec3 x, Vec3 d, Vec3 y, Vec3 yn) const = 0;

protected:
    const Figure &figure;

public:
    FigureLight(const Figure &figure): figure(figure) {}

    float pdf(Vec3 x, Vec3 n, Vec3 d) const override {
        (void) n;

        auto firstIntersection = figure.intersect(Ray(x, d));
        if (!firstIntersection.has_value()) {
            return 0.;
        }
        auto [t, yn, _] = firstIntersection.value();
        if (std::isnan(t)) { // Shouldn't happen actually...
            return INFINITY;
        }
        Vec3 y = x + t * d;
        float ans = pdfOne(x, d, y, yn);

        auto secondIntersection = figure.intersect(Ray(x + (t + 0.0001) * d, d));
        if (!secondIntersection.has_value()) {
            return ans;
        }
        auto [t2, yn2, __] = secondIntersection.value();
        Vec3 y2 = x + (t + 0.0001 + t2) * d;
        return ans + pdfOne(x, d, y2, yn2);
    }
};

class BoxLight : public FigureLight {
private:
    std::uniform_real_distribution<float> u01{0.0, 1.0};
    rng_type &rng;
    float sTotal;
    
    float pdfOne(Vec3 x, Vec3 d, Vec3 y, Vec3 yn) const override {
        return (x - y).len2() / (sTotal * fabs(d.dot(yn)));
    }

public:
    BoxLight(rng_type &rng, const Figure &box): FigureLight(box), rng(rng) {
        float sx = box.data.x, sy = box.data.y, sz = box.data.z;
        sTotal = 8 * (sy * sz + sx * sz + sx * sy);
    }

    Vec3 sample(Vec3 x, Vec3 n) override {
        (void) n;

        float sx = figure.data.x, sy = figure.data.y, sz = figure.data.z;
        float wx = sy * sz;
        float wy = sx * sz;
        float wz = sx * sy;

        while (true) {
            float u = u01(rng) * (wx + wy + wz);
            float flipSign = u01(rng) > 0.5 ? 1 : -1;
            Vec3 point;

            if (u < wx) {
                point = Vec3(flipSign * sx, (2 * u01(rng) - 1) * sy, (2 * u01(rng) - 1) * sz);
            } else if (u < wx + wy) {
                point = Vec3((2 * u01(rng) - 1) * sx, flipSign * sy, (2 * u01(rng) - 1) * sz);
            } else {
                point = Vec3((2 * u01(rng) - 1) * sx, (2 * u01(rng) - 1) * sy, flipSign * sz);
            }

            Vec3 actualPoint = figure.rotation.conjugate().transform(point) + figure.position;
            if (figure.intersect(Ray(x, (actualPoint - x).normalize())).has_value()) {
                return (actualPoint - x).normalize();
            }
        }
    }
};

class EllipsoidLight : public FigureLight {
private:
    std::normal_distribution<float> n01{0.f, 1.f};
    rng_type &rng;
    
    float pdfOne(Vec3 x, Vec3 d, Vec3 y, Vec3 yn) const override {
        Vec3 r = figure.data;
        Vec3 n = figure.rotation.transform(y - figure.position) / r;
        float pointProb = 1. / (4 * PI * Vec3{n.x * r.y * r.z, r.x * n.y * r.z, r.x * r.y * n.z}.len());
        return pointProb * (x - y).len2() / fabs(d.dot(yn));
    }

public:
    EllipsoidLight(rng_type &rng, const Figure &ellipsoid): FigureLight(ellipsoid), rng(rng) {}

    Vec3 sample(Vec3 x, Vec3 n) override {
        (void) n;
        Vec3 r = figure.data;

        while (true) {
            Vec3 point = r * Vec3{n01(rng), n01(rng), n01(rng)}.normalize();
            Vec3 actualPoint = figure.rotation.conjugate().transform(point) + figure.position;
            if (figure.intersect(Ray(x, (actualPoint - x).normalize())).has_value()) {
                return (actualPoint - x).normalize();
            }
        }
    }
};


class Mix : public Distribution {
private:
    std::uniform_real_distribution<float> u01{0.0, 1.0};
    rng_type &rng;
    const std::vector<std::unique_ptr<Distribution>> components;

public:
    Mix(rng_type &rng, std::vector<std::unique_ptr<Distribution>> &&components): rng(rng), components(std::move(components)) {}

    Vec3 sample(Vec3 x, Vec3 n) override {
        int distNum = u01(rng) * components.size();
        return components[distNum]->sample(x, n);
    }

    float pdf(Vec3 x, Vec3 n, Vec3 d) const override {
        float ans = 0;
        for (const auto &component : components) {
            ans += component->pdf(x, n, d);
        }
        return ans / components.size();
    }
};