#include "sceneio.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace sceneio {

Scene loadScene(const std::string &filename) {
    std::ifstream fin(filename);
    Scene scene;

    std::string cmd_line;
    while (getline(fin, cmd_line)) {
        std::stringstream ss;
        ss << cmd_line;
        std::string cmd;
        ss >> cmd;

        if (cmd == "DIMENSIONS") {
            ss >> scene.width >> scene.height;
        } else if (cmd == "BG_COLOR") {
            ss >> scene.bgColor;
        } else {
            std::cerr << "UNKNOWN COMMAND: " << cmd << std::endl;
        }
    }
    return scene;
}

void renderScene(const Scene &scene, const std::string &filename) {
    std::ofstream fout(filename);
    fout << "P6\n";
    fout << scene.width << ' ' << scene.height << '\n';
    fout << 255 << '\n';
    for (int x = 0; x < scene.width; x++) {
        for (int y = 0; y < scene.height; y++) {
            uint8_t *pixel = scene.getPixel(x, y).toExternFormat();
            fout.write((char *) pixel, 3);
        }
    }
    fout.close();
}

}