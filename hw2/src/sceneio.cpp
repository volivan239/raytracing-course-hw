#include "sceneio.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace sceneio {

std::pair<std::unique_ptr<Figure>, std::optional<std::string>> loadPrimitive(std::istream &in) {
    std::string cmdLine;

    getline(in, cmdLine);
    std::unique_ptr<Figure> figure;
    std::stringstream ss;
    ss << cmdLine;
    std::string figureName;
    ss >> figureName;

    if (figureName == "ELLIPSOID") {
        Vec3 r;
        ss >> r;
        figure = std::unique_ptr<Figure>(new Ellipsoid(r));
    } else if (figureName == "PLANE") {
        Vec3 n;
        ss >> n;
        figure = std::unique_ptr<Figure>(new Plane(n));
    } else if (figureName == "BOX") {
        Vec3 s;
        ss >> s;
        figure = std::unique_ptr<Figure>(new Box(s));
    } else {
        std::cerr << "UNKNWOWN FIGURE: " << figureName << '@' << cmdLine << std::endl;
        return std::make_pair(nullptr, cmdLine);
    }

    while (getline(in, cmdLine)) {
        std::string cmd;

        std::stringstream ss;
        ss << cmdLine;
        ss >> cmd;
        if (cmd == "COLOR") {
            ss >> figure->color;
        } else if (cmd == "POSITION") {
            ss >> figure->position;
        } else if (cmd == "ROTATION") {
            ss >> figure->rotation;
        } else if (cmd == "METALLIC") {
            figure->material = Material::METALLIC;
        } else if (cmd == "DIELECTRIC") {
            figure->material = Material::DIELECTRIC;
        } else {
            return std::make_pair(std::move(figure), cmdLine);
        }
    }
    return std::make_pair(std::move(figure), std::nullopt);
}

std::pair<std::unique_ptr<LightSource>, std::optional<std::string>> loadLightSource(std::istream &in) {    
    Color intensity;
    Vec3 direction;
    Vec3 position;
    Vec3 attenuation;
    bool isDirected = false;

    std::string cmdLine;
    std::optional<std::string> nextCmdLine = std::nullopt;
    while (getline(in, cmdLine)) {
        std::string cmd;

        std::stringstream ss;
        ss << cmdLine;
        ss >> cmd;
        if (cmd == "LIGHT_INTENSITY") {
            ss >> intensity;
        } else if (cmd == "LIGHT_POSITION") {
            ss >> position;
        } else if (cmd == "LIGHT_DIRECTION") {
            ss >> direction;
            isDirected = true;
        } else if (cmd == "LIGHT_ATTENUATION") {
            ss >> attenuation;
        } else {
            nextCmdLine = cmdLine;
            break;
        }
    }
    std::unique_ptr<LightSource> lightSource;
    if (isDirected) {
        lightSource = std::unique_ptr<LightSource>(new DirectedLightSource(intensity, direction));
    } else {
        lightSource = std::unique_ptr<LightSource>(new DotLightSource(intensity, position, attenuation));
    }
    return std::make_pair(std::move(lightSource), nextCmdLine);
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
                scene.figures.push_back(std::move(figure));
                if (nextCmd.has_value()) {
                    cmdLine = nextCmd.value();
                    continue;
                }
            } else if (cmd == "RAY_DEPTH") {
                ss >> scene.rayDepth;
            } else if (cmd == "AMBIENT_LIGHT") {
                ss >> scene.ambientLight;
            } else if (cmd == "NEW_LIGHT") {
                auto [lightSource, nextCmd] = loadLightSource(in);
                scene.lightSources.push_back(std::move(lightSource));
                if (nextCmd.has_value()) {
                    cmdLine = nextCmd.value();
                    continue;
                }
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