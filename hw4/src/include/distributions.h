#pragma once
#include <random>
#include <cmath>
#include "vec3.h"

typedef std::minstd_rand rng_type;
const float PI = acos(-1);

class Uniform {
private:
    std::normal_distribution<float> n01{0.f, 1.f};
    std::minstd_rand &rng;

public:
    Uniform(rng_type &rng): rng(rng) {}

    Vec3 sample(Vec3 x, Vec3 n) {
        (void) x;
        Vec3 d = Vec3 {n01(rng), n01(rng), n01(rng)}.normalize();
        if (d.dot(n) < 0) {
            d = -1. * d;
        }
        return d;
    }

    float pdf(Vec3 x, Vec3 n, Vec3 d) const {
        (void) x;
        if (d.dot(n) < 0) {
            return 0;
        }
        return 1. / (2. * PI);
    }
};

class Cosine {
private:
    std::normal_distribution<float> n01{0.f, 1.f};
    std::minstd_rand &rng;
    static constexpr float eps = 1e-9;

public:
    Cosine(rng_type &rng): rng(rng) {}

    Vec3 sample(Vec3 x, Vec3 n) {
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

    float pdf(Vec3 x, Vec3 n, Vec3 d) const {
        (void) x;
        return std::max(0.f, d.dot(n) / PI);
    }
};