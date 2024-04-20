#pragma once

#include "vec3.h"
#include <iostream> // TODO: delete

class MaterialModel {
private:
    float alpha2;
    float metallic;
    Color baseColor;

private:    
    float distributionTerm(const Vec3 &h, const Vec3 &n) const {
        float dotHN = h.dot(n);
        if (dotHN < 0) {
            return 0;
        }
        return alpha2 / (M_PI * pow((alpha2 - 1) * dotHN * dotHN + 1, 2.0));
    }

    float g1(const Vec3 &n, const Vec3 &x, const Vec3 &h) const {
        if (h.dot(x) < 0) {
            return 0;
        }
        return 2 * fabs(n.dot(x)) / (fabs(n.dot(x)) + sqrt(alpha2 + (1 - alpha2) * n.dot(x) * n.dot(x)));
    }

    float geometryTerm(const Vec3 &l, const Vec3 &v, const Vec3 &n) const {
        Vec3 h = (l + v).normalize();
        return g1(n, l, h) * g1(n, v, h);
    }

    float specularBrdf(const Vec3 &l, const Vec3 &v, const Vec3 &n) const {
        Vec3 h = (l + v).normalize();
        return distributionTerm(h, n) * geometryTerm(l, v, n) / (4 * fabs(l.dot(n)) * fabs(v.dot(n)));
    }

    Vec3 diffuseBrdf(const Color &color) const {
        return (1. / M_PI) * color;
    }

    Vec3 fresnelTerm(const Vec3 &f0, const Vec3 &f90, const Vec3 &v, const Vec3 &h) const {
        return f0 + pow(1 - fabs(v.dot(h)), 5.0) * (f90 - f0);
    }

public:
    MaterialModel(float alpha, float metallic, Color baseColor): alpha2(alpha * alpha), metallic(metallic), baseColor(baseColor) {}

    Vec3 brdf(const Vec3 &l, const Vec3 &v, const Vec3 &n) const {
        Vec3 h = (l + v).normalize();
        float specular = specularBrdf(l, v, n);

        Vec3 metalBrdf, dieletricBrdf;
        if (metallic > 0) {
            auto ft = fresnelTerm(baseColor, {1, 1, 1}, v, h);
            metalBrdf = specular * ft;
        }
        if (metallic < 1) {
            Vec3 diffuse = diffuseBrdf(baseColor);
            Vec3 fr = fresnelTerm({0.04, 0.04, 0.04}, {1, 1, 1}, v, h);
            dieletricBrdf = diffuse * (Vec3{1, 1, 1} - fr) + specular * fr; // ???
        }
        return (1.0 - metallic) * dieletricBrdf + metallic * metalBrdf;
    }
};