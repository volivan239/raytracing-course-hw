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


class Distribution {
public:
    virtual Vec3 sample(rng_type &rng, Vec3 x, Vec3 n) = 0;
    virtual float pdf(Vec3 x, Vec3 n, Vec3 d) const = 0;
};


class Uniform : public Distribution {
private:
    std::normal_distribution<float> n01{0.f, 1.f};

public:
    Uniform() {}

    Vec3 sample(rng_type &rng, Vec3 x, Vec3 n) override {
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
    static constexpr float eps = 1e-9;

public:
    Cosine() {}

    Vec3 sample(rng_type &rng, Vec3 x, Vec3 n) override {
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

public:
    const Figure figure;

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
    float sTotal;
    
    float pdfOne(Vec3 x, Vec3 d, Vec3 y, Vec3 yn) const override {
        return (x - y).len2() / (sTotal * fabs(d.dot(yn)));
    }

public:
    BoxLight(const Figure &box): FigureLight(box) {
        float sx = box.data.x, sy = box.data.y, sz = box.data.z;
        sTotal = 8 * (sy * sz + sx * sz + sx * sy);
    }

    Vec3 sample(rng_type &rng, Vec3 x, Vec3 n) override {
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

class TriangleLight : public FigureLight {
private:
    std::uniform_real_distribution<float> u01{0.f, 1.f};
    
    float pdfOne(Vec3 x, Vec3 d, Vec3 y, Vec3 yn) const override {
        const Vec3 &b = figure.data;
        const Vec3 &c = figure.data2;
        Vec3 n = b.cross(c);
        float pointProb = 1.0 / (0.5 * n.len());
        return pointProb * (x - y).len2() / fabs(d.dot(yn));
    }

public:
    TriangleLight(const Figure &ellipsoid): FigureLight(ellipsoid) {}

    Vec3 sample(rng_type &rng, Vec3 x, Vec3 n) override {
        (void) n;
        const Vec3 &b = figure.data;
        const Vec3 &c = figure.data2;
        float u = u01(rng);
        float v = u01(rng);
        if (u + v > 1.) {
            u = 1 - u;
            v = 1 - v;
        }
        Vec3 point = figure.position + figure.rotation.conjugate().transform(u * b + v * c);
        return (point - x).normalize();
    }
};

class EllipsoidLight : public FigureLight {
private:
    std::normal_distribution<float> n01{0.f, 1.f};
    
    float pdfOne(Vec3 x, Vec3 d, Vec3 y, Vec3 yn) const override {
        Vec3 r = figure.data;
        Vec3 n = figure.rotation.transform(y - figure.position) / r;
        float pointProb = 1. / (4 * PI * Vec3{n.x * r.y * r.z, r.x * n.y * r.z, r.x * r.y * n.z}.len());
        return pointProb * (x - y).len2() / fabs(d.dot(yn));
    }

public:
    EllipsoidLight(const Figure &ellipsoid): FigureLight(ellipsoid) {}

    Vec3 sample(rng_type &rng, Vec3 x, Vec3 n) override {
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
    const std::vector<std::unique_ptr<Distribution>> components;

public:
    Mix(std::vector<std::unique_ptr<Distribution>> &&components): components(std::move(components)) {}

    Vec3 sample(rng_type &rng, Vec3 x, Vec3 n) override {
        int distNum = u01(rng) * components.size();
        return components[distNum]->sample(rng, x, n);
    }

    float pdf(Vec3 x, Vec3 n, Vec3 d) const override {
        float ans = 0;
        for (const auto &component : components) {
            ans += component->pdf(x, n, d);
        }
        return ans / components.size();
    }
};

class FiguresMix : public Distribution {
private:
    std::uniform_real_distribution<float> u01{0.0, 1.0};
    std::vector<std::variant<BoxLight, EllipsoidLight, TriangleLight>> figures_;
    BVH bvh;

public:
    FiguresMix(std::vector<Figure> figures) {
        size_t n = std::partition(figures.begin(), figures.end(), [](const auto &fig) {
            if (fig.emission.x == 0 && fig.emission.y == 0 && fig.emission.z == 0) {
                return false;
            }
            return fig.type == FigureType::BOX || fig.type == FigureType::ELLIPSOID || fig.type == FigureType::TRIANGLE;
        }) - figures.begin();

        bvh = BVH(figures, n);
        for (size_t i = 0; i < n; i++) {
            if (figures[i].type == FigureType::BOX) {
                figures_.push_back(BoxLight(figures[i]));
            } else if (figures[i].type == FigureType::ELLIPSOID) {
                figures_.push_back(EllipsoidLight(figures[i]));
            } else {
                figures_.push_back(TriangleLight(figures[i]));
            }
        }
    }

    Vec3 sample(rng_type &rng, Vec3 x, Vec3 n) override {
        int distNum = u01(rng) * figures_.size();
        if (std::holds_alternative<BoxLight>(figures_[distNum])) {
            return std::get<BoxLight>(figures_[distNum]).sample(rng, x, n);
        } else if (std::holds_alternative<EllipsoidLight>(figures_[distNum])) {
            return std::get<EllipsoidLight>(figures_[distNum]).sample(rng, x, n);
        } else {
            return std::get<TriangleLight>(figures_[distNum]).sample(rng, x, n);
        }
    }

    float pdf(Vec3 x, Vec3 n, Vec3 d) const override {
        return getTotalPdf(0, x, n, d) / figures_.size();
    }

    bool isEmpty() const {
        return figures_.empty();
    }

private:
    float getTotalPdf(uint32_t pos, const Vec3 &x, const Vec3 &n, const Vec3 &d) const {
        Ray ray(x, d);
        const Node &cur = bvh.nodes[pos];
        auto intersection = cur.aabb.intersect(ray);
        if (!intersection.has_value()) {
            return 0;
        }    

        if (cur.left == 0) {
            float result = 0;
            for (uint32_t i = cur.first; i < cur.last; i++) {
                if (std::holds_alternative<BoxLight>(figures_[i])) {
                    result += std::get<BoxLight>(figures_[i]).pdf(x, n, d);
                } else if (std::holds_alternative<EllipsoidLight>(figures_[i])) {
                    result += std::get<EllipsoidLight>(figures_[i]).pdf(x, n, d);
                } else {
                    result += std::get<TriangleLight>(figures_[i]).pdf(x, n, d);
                }
            }
            return result;
        }

        return getTotalPdf(cur.left, x, n, d) + getTotalPdf(cur.right, x, n, d);
    }    
};