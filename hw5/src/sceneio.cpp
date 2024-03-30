#include "sceneio.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace sceneio {

const int SAMPLES_DROP = 4;

std::pair<Figure, std::optional<std::string>> loadPrimitive(std::istream &in) {
    std::string cmdLine;
    Figure figure;
    while (getline(in, cmdLine)) {
        std::string cmd;

        std::stringstream ss;
        ss << cmdLine;
        ss >> cmd;
        if (cmd == "ELLIPSOID") {
            ss >> figure.data;
            figure.type = FigureType::ELLIPSOID;
        } else if (cmd == "PLANE") {
            ss >> figure.data;
            figure.type = FigureType::PLANE;
        } else if (cmd == "BOX") {
            ss >> figure.data;
            figure.type = FigureType::BOX;
        } else if (cmd == "TRIANGLE") {
            ss >> figure.data3 >> figure.data2 >> figure.data;
            figure.type = FigureType::TRIANGLE;
        } else if (cmd == "COLOR") {
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
                scene.samples /= SAMPLES_DROP;
            } else {
                if (cmd != "") {
                    std::cerr << "UNKNOWN COMMAND: " << cmd << std::endl;
                }
            }
            break;
        }
    }
    scene.initBVH();
    scene.initDistribution();
    return scene;
}

void renderScene(const Scene &scene, std::ostream &out) {
    out << "P6\n";
    out << scene.width << ' ' << scene.height << '\n';
    out << 255 << '\n';
    std::array<uint8_t, 3> result[scene.height][scene.width];
    #pragma omp parallel for schedule(dynamic,8)
    for (int i = 0; i < scene.height * scene.width; i++) {
        rng_type rng(i);
        int x = i % scene.width;
        int y = i / scene.width;
        auto pixel0 = scene.getPixel(rng, x, y);
        result[y][x] = toExternColorFormat(
            gamma_corrected(aces_tonemap(pixel0))
        );
    }
    for (int y = 0; y < scene.height; y++) {
        for (int x = 0; x < scene.width; x++) {
            out.write(reinterpret_cast<char*>(result[y][x].data()), 3);
        }
    }
}

}