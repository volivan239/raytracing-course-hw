#pragma once
#include <iostream>
#include "scene.h"

namespace sceneio {

void renderScene(Scene &scene, std::string_view outFileName);
Scene loadScene(std::string_view gltfFilename);

}
