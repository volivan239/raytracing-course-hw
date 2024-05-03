#include <fstream>
#include "scene.h"
#include "sceneio.h"

using namespace std;

int main(int argc, const char *argv[]) {
    Scene scene = sceneio::loadScene(argv[1]);
    scene.width = strtol(argv[2], nullptr, 10);
    scene.height = strtol(argv[3], nullptr, 10);
    scene.samples = strtol(argv[4], nullptr, 10) * 2;
    if (argc > 6) {
        scene.environmentMap = sceneio::loadTexture(argv[6]); // TODO: or true?
    }
    sceneio::renderScene(scene, argv[5]);
    std::cerr << "FINISH" << std::endl;
    return 0;
}