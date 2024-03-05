#pragma once
#include <random>
#include "vec3.h"

typedef std::minstd_rand rng_type;
const float PI = acos(-1);

class Uniform {
private:
    std::minstd_rand &rng;
    std::normal_distribution<float> n01;

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
    std::minstd_rand &rng;
    std::normal_distribution<float> n01;

public:
    Cosine(rng_type &rng): rng(rng) {}

    Vec3 sample(Vec3 x, Vec3 n) {
        (void) x;
        Vec3 d = Vec3 {n01(rng), n01(rng), n01(rng)}.normalize();
        return (d + n).normalize();
    }

    float pdf(Vec3 x, Vec3 n, Vec3 d) const {
        (void) x;
        return std::max(0.f, d.dot(n) / PI);
    }
};