#pragma once
#include <iostream>
#include "scene.h"

namespace sceneio {

Texture loadTexture(std::string_view file);
void renderScene(Scene &scene, std::string_view outFileName);
Scene loadScene(std::string_view gltfFilename);

}
