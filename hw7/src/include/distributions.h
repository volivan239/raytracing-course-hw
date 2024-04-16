#pragma once
#include <random>
#include <cmath>
#include <memory>
#include <variant>
#include "vec3.h"
#include "primitives.h"
#include "bvh.h"

typedef std::minstd_rand rng_type;
const float PI = acos(-1);

class Uniform {
public:
    Uniform() {}

    Vec3 sample(std::normal_distribution<float> &n01, rng_type &rng, Vec3 x, Vec3 n) {
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
    static constexpr float eps = 1e-9;

public:
    Cosine() {}

    Vec3 sample(std::normal_distribution<float> &n01, rng_type &rng, Vec3 x, Vec3 n) {
        (void) x;
        Vec3 d = Vec3 {n01(rng), n01(rng), n01(rng)}.normalize();
        d = d + n;
        float len = d.len();
        if (len <= eps || d.dot(n) <= eps || std::isnan(len)) {
            // Can happen due to precision errors
            return n;
        }
        return 1. / len * d;
    }

    float pdf(Vec3 x, Vec3 n, Vec3 d) const {
        (void) x;
        return std::max(0.f, d.dot(n) / PI);
    }
};

class TriangleLight {
private:
    float pointProb;

public:
    const Figure figure;
    
public:
    float pdfOne(Vec3 x, Vec3 d, Vec3 y, Vec3 yn) const {
        return pointProb * (x - y).len2() / fabs(d.dot(yn));
    }

public:
    TriangleLight(const Figure &ellipsoid): figure(ellipsoid) {
        const Vec3 &a = figure.data3.coords;
        const Vec3 &b = figure.data.coords - a;
        const Vec3 &c = figure.data2.coords - a;
        Vec3 n = b.cross(c);
        pointProb = 1.0 / (0.5 * n.len());
    }

    Vec3 sample(std::uniform_real_distribution<float> &u01, rng_type &rng, Vec3 x, Vec3 n) {
        (void) n;
        const Vec3 &a = figure.data3.coords;
        const Vec3 &b = figure.data.coords - a;
        const Vec3 &c = figure.data2.coords - a;
        float u = u01(rng);
        float v = u01(rng);
        if (u + v > 1.) {
            u = 1 - u;
            v = 1 - v;
        }
        Vec3 point = figure.position + figure.rotation.conjugate().transform(a + u * b + v * c);
        return (point - x).normalize();
    }
};

class FiguresMix {
private:
    std::vector<TriangleLight> figures_;
    BVH bvh;

public:
    FiguresMix(std::vector<Figure> figures) {
        size_t n = std::partition(figures.begin(), figures.end(), [](const auto &fig) {
            if (fig.material.emission.x == 0 && fig.material.emission.y == 0 && fig.material.emission.z == 0) {
                return false;
            }
            return true;
        }) - figures.begin();

        bvh = BVH(figures, n);
        for (size_t i = 0; i < n; i++) {
            figures_.push_back(TriangleLight(figures[i]));
        }
    }

    Vec3 sample(std::uniform_real_distribution<float> &u01, rng_type &rng, Vec3 x, Vec3 n) {
        int distNum = u01(rng) * figures_.size();
        return figures_[distNum].sample(u01, rng, x, n);
    }

    float pdf(Vec3 x, Vec3 n, Vec3 d) const {
        return getTotalPdf(0, x, n, d) / figures_.size();
    }

    bool isEmpty() const {
        return figures_.empty();
    }

private:
    float pdfOneFigureLight(const TriangleLight &figureLight, Vec3 x, Vec3 n, Vec3 d) const {
        (void) n;

        const Figure &figure = figureLight.figure;

        auto firstIntersection = figure.intersect(Ray(x, d));
        if (!firstIntersection.has_value()) {
            return 0.;
        }
        auto [t, yn, _, __] = firstIntersection.value();
        if (std::isnan(t)) { // Shouldn't happen actually...
            return INFINITY;
        }
        Vec3 y = x + t * d;
        return figureLight.pdfOne(x, d, y, yn);
    }

    float getTotalPdf(uint32_t pos, const Vec3 &x, const Vec3 &n, const Vec3 &d) const {
        Ray ray(x, d);
        const BvhNode &cur = bvh.nodes[pos];
        auto intersection = cur.aabb.intersect(ray);
        if (!intersection.has_value()) {
            return 0;
        }    

        if (cur.left == 0) {
            float result = 0;
            for (uint32_t i = cur.first; i < cur.last; i++) {
                result += pdfOneFigureLight(figures_[i], x, n, d);
            }
            return result;
        }

        return getTotalPdf(cur.left, x, n, d) + getTotalPdf(cur.right, x, n, d);
    }    
};

class Mix {
private:
    std::vector<std::variant<Cosine, FiguresMix>> components;

public:
    Mix() {}
    Mix(const std::vector<std::variant<Cosine, FiguresMix>> &components): components(components) {}

    Vec3 sample(std::uniform_real_distribution<float> &u01, std::normal_distribution<float> &n01, rng_type &rng, Vec3 x, Vec3 n) {
        int distNum = u01(rng) * components.size();
        if (std::holds_alternative<Cosine>(components[distNum])) {
            return std::get<Cosine>(components[distNum]).sample(n01, rng, x, n);
        } else {
            return std::get<FiguresMix>(components[distNum]).sample(u01, rng, x, n);
        }
    }

    float pdf(Vec3 x, Vec3 n, Vec3 d) const {
        float ans = 0;
        for (const auto &component : components) {
            if (std::holds_alternative<Cosine>(component)) {
                ans += std::get<Cosine>(component).pdf(x, n, d);
            } else {
                ans += std::get<FiguresMix>(component).pdf(x, n, d);
            }
        }
        return ans / components.size();
    }
};