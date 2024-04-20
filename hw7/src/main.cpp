#include <fstream>
#include "scene.h"
#include "sceneio.h"

using namespace std;

int main(int argc, const char *argv[]) {
    (void) argc;

    Scene scene = sceneio::loadScene(argv[1]);
    scene.width = strtol(argv[2], nullptr, 10);
    scene.height = strtol(argv[3], nullptr, 10);
    scene.samples = strtol(argv[4], nullptr, 10) * 0.75;
    sceneio::renderScene(scene, argv[5]);
    std::cerr << "FINISH" << std::endl;
    return 0;
}