#include <iostream>
#include "scene.h"
#include "sceneio.h"

using namespace std;

int main(int argc, const char *argv[]) {
    (void) argc;

    string input_file(argv[1]);
    string output_file(argv[2]);
    Scene scene = sceneio::loadScene(input_file);
    sceneio::renderScene(scene, output_file);
    return 0;
}