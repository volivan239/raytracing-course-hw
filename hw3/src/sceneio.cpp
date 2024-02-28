#include "sceneio.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace sceneio {

std::pair<Figure, std::optional<std::string>> loadPrimitive(std::istream &in) {
    std::string cmdLine;

    getline(in, cmdLine);
    Figure figure;
    std::stringstream ss;
    ss << cmdLine;
    std::string figureName;
    ss >> figureName;

    if (figureName == "ELLIPSOID") {
        Vec3 r;
        ss >> r;
        figure = Figure(FigureType::ELLIPSOID, r);
    } else if (figureName == "PLANE") {
        Vec3 n;
        ss >> n;
        figure = Figure(FigureType::PLANE, n.normalize());
    } else if (figureName == "BOX") {
        Vec3 s;
        ss >> s;
        figure = Figure(FigureType::BOX, s);
    } else {
        std::cerr << "UNKNWOWN FIGURE: " << figureName << '@' << cmdLine << std::endl;
        return std::make_pair(figure, cmdLine);
    }

    while (getline(in, cmdLine)) {
        std::string cmd;

        std::stringstream ss;
        ss << cmdLine;
        ss >> cmd;
        if (cmd == "COLOR") {
            ss >> figure.color;
        } else if (cmd == "POSITION") {
            ss >> figure.position;
        } else if (cmd == "ROTATION") {
            ss >> figure.rotation;
        } else if (cmd == "METALLIC") {
            figure.material = Material::METALLIC;
        } else if (cmd == "DIELECTRIC") {
            figure.material = Material::DIELECTRIC;
        } else if (cmd == "EMISSION") {
            ss >> figure.emission;
        } else if (cmd == "IOR") {
            ss >> figure.ior;
        } else {
            return std::make_pair(figure, cmdLine);
        }
    }
    return std::make_pair(figure, std::nullopt);
}

Scene loadScene(std::istream &in) {
    Scene scene;

    std::string cmdLine;
    while (getline(in, cmdLine)) {
        while (true) {
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
                auto [figure, nextCmd] = loadPrimitive(in);
                scene.figures.push_back(figure);
                if (nextCmd.has_value()) {
                    cmdLine = nextCmd.value();
                    continue;
                }
            } else if (cmd == "RAY_DEPTH") {
                ss >> scene.rayDepth;
            } else if (cmd == "SAMPLES") {
                ss >> scene.samples;
            } else {
                if (cmd != "") {
                    std::cerr << "UNKNOWN COMMAND: " << cmd << std::endl;
                }
            }
            break;
        }
    }
    return scene;
}

void renderScene(const Scene &scene, std::ostream &out) {
    out << "P6\n";
    out << scene.width << ' ' << scene.height << '\n';
    out << 255 << '\n';
    for (int y = 0; y < scene.height; y++) {
        for (int x = 0; x < scene.width; x++) {
            uint8_t *pixel = toExternColorFormat(
                gamma_corrected(aces_tonemap(scene.getPixel(x, y)))
            );
            out.write((char *) pixel, 3);
        }
    }
}

}