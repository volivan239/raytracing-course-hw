#include "light_source.h"

LightSource::LightSource() {}
LightSource::LightSource(Color intensity): intensity(intensity) {}

DotLightSource::DotLightSource() {}
DotLightSource::DotLightSource(Color intensity, Vec3 position, Vec3 attenuation): LightSource(intensity), position(position), attenuation(attenuation) {}

std::tuple<Vec3, Color, float> DotLightSource::lightAt(Vec3 point) const {
    Vec3 direction = position - point;
    float r = direction.len();
    Color c = 1. / (attenuation.x + attenuation.y * r + attenuation.z * r * r) * intensity;
    return std::tuple(direction.normalize(), c, r);
}


DirectedLightSource::DirectedLightSource() {}
DirectedLightSource::DirectedLightSource(Color intensity, Vec3 direction): LightSource(intensity), direction(direction) {}

std::tuple<Vec3, Color, float> DirectedLightSource::lightAt(Vec3 point) const {
    (void) point;
    return std::tuple(direction.normalize(), intensity, 1. / 0.);
}