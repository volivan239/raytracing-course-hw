#include "scene.h"
#include <cassert>
#include <fstream>
#include <sstream>
#include <iostream>

Scene::Scene() {}

Color Scene::getPixel(int x, int y) const {
    assert(0 <= x && x < width);
    assert(0 <= y && y < height);
    return bgColor;
}