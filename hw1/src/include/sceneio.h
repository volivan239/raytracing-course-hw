#pragma once
#include <string>
#include "scene.h"

namespace sceneio {

void renderScene(const Scene &scene, const std::string &filename);
Scene loadScene(const std::string &filename);

}
