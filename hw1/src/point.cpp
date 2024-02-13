#include "point.h"
#include <math.h>

Point::Point() {}

Point::Point(float x, float y, float z): x(x), y(y), z(z) {}

Point Point::operator + (const Point &other) const {
    return Point(x + other.x, y + other.y, z + other.z);
}

Point Point::operator - (const Point &other) const {
    return Point(x - other.x, y - other.y, z - other.z);
}

float Point::operator * (const Point &other) const {
    return x * other.x + y * other.y + z * other.z;
}

float Point::len() const {
    return sqrt((*this) * (*this));
}

std::istream& operator >> (std::istream &in, Point &point) {
    in >> point.x >> point.y >> point.z;
    return in;
}
