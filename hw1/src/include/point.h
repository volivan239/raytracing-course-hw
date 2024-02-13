#include <istream>

#pragma once

class Point {
public:
    float x, y, z;
    Point();
    Point(float x, float y, float z);

    Point operator + (const Point &other) const;
    Point operator - (const Point &other) const;
    float operator * (const Point &other) const;
    float len() const;
};

std::istream& operator >> (std::istream &in, Point &point);