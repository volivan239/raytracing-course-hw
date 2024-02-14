#include "sceneio.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace sceneio {

Scene loadScene(const std::string &filename) {
    std::ifstream fin(filename);
    Scene scene;

    std::string cmdLine;
    while (getline(fin, cmdLine)) {
        std::stringstream ss;
        ss << cmdLine;
        std::string cmd;
        ss >> cmd;

        if (cmd == "DIMENSIONS") {
            ss >> scene.width >> scene.height;
        } else if (cmd == "BG_COLOR") {
            ss >> scene.bgColor;
        } else if (cmd == "CAMERA_POSITION") {
            ss >> scene.cameraPos;
        } else if (cmd == "CAMERA_RIGHT") {
            ss >> scene.cameraRight;
        } else if (cmd == "CAMERA_UP") {
            ss >> scene.cameraUp;
        } else if (cmd == "CAMERA_FORWARD") {
            ss >> scene.cameraForward;
        } else if (cmd == "CAMERA_FOV_X") {
            ss >> scene.cameraFovX;
        } else if (cmd == "NEW_PRIMITIVE") {
            getline(fin, cmdLine);
            std::unique_ptr<Figure> figure;
            std::stringstream ss;
            ss << cmdLine;
            std::string figureName;
            ss >> figureName;

            if (figureName == "ELLIPSOID") {
                Point r;
                ss >> r;
                figure = std::unique_ptr<Figure>(new Ellipsoid(r));
            } else if (figureName == "PLANE") {
                Point n;
                ss >> n;
                figure = std::unique_ptr<Figure>(new Plane(n));
            } else {
                std::cerr << "UNKNWOWN FIGURE: " << figureName << '@' << cmdLine << std::endl;
                continue;
            }

            while (getline(fin, cmdLine) && cmdLine != "") {
                std::stringstream ss;
                ss << cmdLine;
                ss >> cmd;
                if (cmd == "COLOR") {
                    ss >> figure->color;
                } else if (cmd == "POSITION") {
                    ss >> figure->position;
                } else if (cmd == "ROTATION") {
                    ss >> figure->rotation;
                } else {
                    std::cerr << "UNKNOWN FIGURE PARAMETER: " << cmd << std::endl;
                    break;
                }
            }
            scene.figures.push_back(std::move(figure));
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
    for (int y = 0; y < scene.height; y++) {
        for (int x = 0; x < scene.width; x++) {
            uint8_t *pixel = scene.getPixel(x, y).toExternFormat();
            fout.write((char *) pixel, 3);
        }
    }
    fout.close();
}

}