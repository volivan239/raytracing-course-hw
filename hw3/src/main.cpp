#include <fstream>
#include "scene.h"
#include "sceneio.h"

using namespace std;

int main(int argc, const char *argv[]) {
    (void) argc;

    std::ifstream fin(argv[1]);
    std::ofstream fout(argv[2], std::ios::binary);
    Scene scene = sceneio::loadScene(fin);
    sceneio::renderScene(scene, fout);
    return 0;
}