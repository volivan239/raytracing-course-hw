#pragma once
#include <random>
#include <cmath>
#include <memory>
#include <variant>
#include "vec3.h"
#include "primitives.h"
#include "bvh.h"

typedef std::minstd_rand rng_type;

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
        return 1. / (2. * M_PI);
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
        return std::max(0.f, d.dot(n) / (float) M_PI);
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

class Vndf {
private:
    Vec3 sample_(std::uniform_real_distribution<float> &u01, rng_type &rng, Vec3 v, float alpha_) const {
        // Section 3.2: transforming the view direction to the hemisphere configuration
        Vec3 vh = Vec3(alpha_ * v.x, alpha_ * v.y, v.z).normalize();

        // Section 4.1: orthonormal basis (with special case if cross product is zero)
        float lensq = vh.x * vh.x + vh.y * vh.y;
        Vec3 T1 = lensq > 0 ? 1. / sqrt(lensq) * Vec3(-vh.y, vh.x, 0) : Vec3(1, 0, 0);
        Vec3 T2 = T1.cross(vh);

        // Section 4.2: parameterization of the projected area
        float u1 = u01(rng), u2 = u01(rng);
        float r = sqrt(u1);
        float phi = 2.0 * M_PI * u2;
        float t1 = r * cos(phi);
        float t2 = r * sin(phi);
        float s = 0.5 * (1.0 + vh.z);
        t2 = (1.0 - s) * sqrt(1.f - t1 * t1) + s * t2;

        // Section 4.3: reprojection onto hemisphere
        Vec3 nh = t1 * T1 + t2 * T2 + sqrt(std::max<float>(0.f, 1.0 - t1 * t1 - t2 * t2)) * vh;

        // Section 3.4: transforming the normal back to the ellipsoid configuration
        Vec3 ne = Vec3(alpha_ * nh.x, alpha_ * nh.y, std::max<float>(0.0, nh.z)).normalize();
        if (alpha_ < 3e-3 && ne.z < 0.8) {
            std::cerr << "FUCK" << std::endl;
        }
        return 2 * ne.dot(v) * ne - v;
    }

    float D(Vec3 n, float alpha_) const {
        return 1. / (M_PI * alpha_ * alpha_ * pow(n.x * n.x / (alpha_ * alpha_) + n.y * n.y / (alpha_ * alpha_) + n.z * n.z, 2.0));
    }

    float G1(Vec3 v, float alpha_) const {
        float lambda = 0.5 * (-1 + sqrt(1 + (alpha_ * alpha_ * v.x * v.x + alpha_ * alpha_ * v.y * v.y) / (v.z * v.z)));
        return 1. / (1 + lambda);
    }

    float pdf_(Vec3 d, Vec3 v, float alpha_) const {
        Vec3 ni = (v + d).normalize();
        float dv = G1(v, alpha_) * std::max(0.f, v.dot(ni)) * D(ni, alpha_) / fabs(v.z);
        float res = dv / (4 * v.dot(ni));
        if (std::isnan(v.z)) {
            std::cerr << "After: " << v.x << ' ' << v.y << ' ' << v.z << std::endl;
        }
        return res;
    }

    Quaternion getQ(Vec3 n) const {
        Vec3 newN = {0, 0, 1};
        if (n.dot(newN) > 0.9999) {
            return {};
        }
        if (n.dot(newN) < -0.9999) {
            return Quaternion{0, 0, 0, -1};
        }
        Vec3 a = n.cross(newN);
        float w = sqrt(n.len2()) + n.dot(newN);
        float len = sqrt(a.len2() + w * w);
        return Quaternion{1. / len * a, w / len};
    }

public:
    Vndf() {}

    Vec3 sample(std::uniform_real_distribution<float> &u01, rng_type &rng, Vec3 x, Vec3 n, Vec3 v, float alpha_) const {
        (void) x;
        v = -1. * v;
        auto q = getQ(n);
        auto vTransformed = q.transform(v);
        auto dTransformed = sample_(u01, rng, vTransformed, alpha_);
        Vec3 res = q.conjugate().transform(dTransformed);
        return res;
    }

    float pdf(Vec3 x, Vec3 n, Vec3 d, Vec3 v, float alpha_) const {
        (void) x;
        v = -1. * v;
        auto q = getQ(n);
        float res = pdf_(q.transform(d), q.transform(v), alpha_);
        // if (std::isnan(res) || std::isinf(res)) {
        //     std::cerr << "LOSHARA" << ' ' << std::isnan(res) << ' ' << std::isinf(res) << std::endl;
        // }
        return res;
    }
};

class Mix {
private:
    std::vector<std::variant<Cosine, Vndf, FiguresMix>> components;

public:
    Mix() {}
    Mix(const std::vector<std::variant<Cosine, Vndf, FiguresMix>> &components): components(components) {}

    Vec3 sample(std::uniform_real_distribution<float> &u01, std::normal_distribution<float> &n01, rng_type &rng, Vec3 x, Vec3 n, Vec3 v, float alpha) {
        int distNum = u01(rng) * components.size();
        if (std::holds_alternative<Cosine>(components[distNum])) {
            return std::get<Cosine>(components[distNum]).sample(n01, rng, x, n);
        } else if (std::holds_alternative<FiguresMix>(components[distNum])) {
            return std::get<FiguresMix>(components[distNum]).sample(u01, rng, x, n);
        } else {
            return std::get<Vndf>(components[distNum]).sample(u01, rng, x, n, v, alpha);   
        }
    }

    float pdf(Vec3 x, Vec3 n, Vec3 d, Vec3 v, float alpha) const {
        float ans = 0;
        for (const auto &component : components) {
            if (std::holds_alternative<Cosine>(component)) {
                ans += std::get<Cosine>(component).pdf(x, n, d);
            } else if (std::holds_alternative<FiguresMix>(component)) {
                ans += std::get<FiguresMix>(component).pdf(x, n, d);
            } else {
                ans += std::get<Vndf>(component).pdf(x, n, d, v, alpha);
            }
        }
        return ans / components.size();
    }
};