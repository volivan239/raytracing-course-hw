#pragma once
#include <iostream>
#include "scene.h"

namespace sceneio {

void renderScene(Scene &scene, std::ostream &out);
Scene loadScene(std::istream &in);

}
