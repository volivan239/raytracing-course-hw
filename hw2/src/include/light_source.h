#pragma once
#include <color.h>

class LightSource {
public:
    Color intensity;
    
    virtual std::tuple<Vec3, Color, float> lightAt(Vec3 point) const = 0;

    LightSource();
    LightSource(Color intensity);
};

class DotLightSource : public LightSource {
public:
    Vec3 position;
    Vec3 attenuation;

    DotLightSource();
    DotLightSource(Color intensity, Vec3 position, Vec3 attenuation);
    virtual std::tuple<Vec3, Color, float> lightAt(Vec3 point) const override;
};

class DirectedLightSource : public LightSource {
public:
    Vec3 direction;

    DirectedLightSource();
    DirectedLightSource(Color intensity, Vec3 direction);
    virtual std::tuple<Vec3, Color, float> lightAt(Vec3 point) const override;
};