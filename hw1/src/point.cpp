#include "point.h"
#include <math.h>

Point::Point() {}

Point::Point(float x, float y, float z): x(x), y(y), z(z) {}

Point Point::operator + (const Point &other) const {
    return {x + other.x, y + other.y, z + other.z};
}

Point Point::operator - (const Point &other) const {
    return {x - other.x, y - other.y, z - other.z};
}

Point Point::operator / (const Point &other) const {
    return {x / other.x, y / other.y, z / other.z};
}

float Point::operator * (const Point &other) const {
    return x * other.x + y * other.y + z * other.z;
}

Point Point::operator ^ (const Point &other) const {
    return {z * other.y - y * other.z, x * other.z - z * other.x, y * other.x - x * other.y};
}

float Point::len2() const {
    return (*this) * (*this);
}

float Point::len() const {
    return sqrt(len2());
}

std::istream& operator >> (std::istream &in, Point &point) {
    in >> point.x >> point.y >> point.z;
    return in;
}

Point operator * (float k, const Point &p) {
    return {k * p.x, k * p.y, k * p.z};
}