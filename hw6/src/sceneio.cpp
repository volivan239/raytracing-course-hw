#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "sceneio.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

namespace sceneio {

void loadBuffers(std::string_view gltfFilename, const rapidjson::Document &gltfScene, Scene &scene) {
    const auto &bufferSpecs = gltfScene["buffers"].GetArray();
    for (const auto &bufSpec : bufferSpecs) {
        size_t sz = bufSpec["byteLength"].GetUint();
        Buffer buf(sz);
        
        const auto gltfFilePath = std::filesystem::path(gltfFilename);
        const auto bufferFilePath = gltfFilePath.parent_path().append(bufSpec["uri"].GetString());
        std::ifstream bufferStream(bufferFilePath, std::ios::binary);
        bufferStream.read(buf.data(), sz);
        scene.buffers.push_back(buf);
    }
}

void loadBufferViews(const rapidjson::Document &gltfScene, Scene &scene) {
    const auto &bufferViewSpecs = gltfScene["bufferViews"].GetArray();
    for (const auto &bufferViewSpec : bufferViewSpecs) {
        scene.bufferViews.push_back(BufferView{
            bufferViewSpec["buffer"].GetUint(),
            bufferViewSpec["byteLength"].GetUint(),
            bufferViewSpec["byteOffset"].GetUint(),
            bufferViewSpec["target"].GetUint()
        });
    }
}

void loadNodes(const rapidjson::Document &gltfScene, Scene &scene) {
    const auto &nodes = gltfScene["nodes"].GetArray();
    for (const auto &node : nodes) {
        Node curNode;
        if (node.HasMember("mesh")) {
            curNode.mesh = node["mesh"].GetUint();
        }
        if (node.HasMember("camera")) {
            curNode.camera = node["camera"].GetUint();
        }
        if (node.HasMember("rotation")) {
            auto rotation = node["rotation"].GetArray();
            curNode.rotation = {rotation[0].GetFloat(), rotation[1].GetFloat(), rotation[2].GetFloat(), rotation[3].GetFloat()};
        }
        if (node.HasMember("translation")) {
            auto translation = node["translation"].GetArray();
            curNode.translation = {translation[0].GetFloat(), translation[1].GetFloat(), translation[2].GetFloat()};
        }
        if (node.HasMember("scale")) {
            auto scale = node["scale"].GetArray();
            curNode.scale = {scale[0].GetFloat(), scale[1].GetFloat(), scale[2].GetFloat()};
        }
        if (node.HasMember("children")) {
            auto children = node["children"].GetArray();
            for (const auto &child : children) {
                curNode.children.push_back(child.GetUint());
            }
        }
        if (!curNode.transition.has_value()) {
            curNode.transition = Transition(curNode.translation, curNode.rotation, curNode.scale);
        }
        scene.nodes.push_back(curNode);
    }
}

void loadMeshes(const rapidjson::Document &gltfScene, Scene &scene) {
    const auto &meshes = gltfScene["meshes"].GetArray();
    for (const auto &mesh : meshes) {
        Mesh curMesh;
        for (const auto &primitive : mesh["primitives"].GetArray()) {
            curMesh.primitives.push_back(Primitive{
                primitive["attributes"]["POSITION"].GetUint(),
                primitive["indices"].GetUint(),
                primitive["material"].GetUint()
            });
        }
        scene.meshes.push_back(curMesh);
    }
}

void loadAccessors(const rapidjson::Document &gltfScene, Scene &scene) {
    const auto &accessors = gltfScene["accessors"].GetArray();
    for (const auto &accessor : accessors) {
        scene.accessors.push_back(Accessor{
            accessor["bufferView"].GetUint(),
            accessor["count"].GetUint(),
            accessor["componentType"].GetUint(),
            accessor["type"].GetString()
        });
    }
}

void restoreNodeParents(Scene &scene) {
    for (size_t i = 0; i < scene.nodes.size(); i++) {
        for (const auto &child : scene.nodes[i].children) {
            scene.nodes[child].parentNode = i;
        }
    }
}

void calculateTransitions(Scene &scene) {
    for (auto &node : scene.nodes) {
        node.totalTransition = node.transition.value();
        auto par = node.parentNode;
        while (par.has_value()) {
            node.totalTransition = scene.nodes[par.value()].transition.value().compose(node.totalTransition);
            par = scene.nodes[par.value()].parentNode;
        }
    }
}

std::vector<Vec3> loadPositions(size_t positionsIndex, Scene &scene) {
    std::vector<Vec3> positions;
    const auto &accessor = scene.accessors[positionsIndex];
    const auto &bufferView = scene.bufferViews[accessor.bufferView];
    const auto &buffer = scene.buffers[bufferView.buffer];
    if (accessor.type != "VEC3") {
        std::cerr << "Load positions accessor: " << accessor.type << std::endl;
    }
    for (size_t i = 0; i < accessor.count; i++) {
        Vec3 position;
        position.x = *(reinterpret_cast<const float*>(buffer.data() + bufferView.byteOffset + 12 * i));
        position.y = *(reinterpret_cast<const float*>(buffer.data() + bufferView.byteOffset + 12 * i + 4));
        position.z = *(reinterpret_cast<const float*>(buffer.data() + bufferView.byteOffset + 12 * i + 8));
        positions.push_back(position);
    }
    return positions;
}

void loadMaterials(const rapidjson::Document &gltfScene, Scene &scene) {
    const auto &materials = gltfScene["materials"].GetArray();
    for (const auto &material : materials) {
        GltfMaterial curMaterial;
        if (material.HasMember("pbrMetallicRoughness")) {
            if (material["pbrMetallicRoughness"].HasMember("baseColorFactor")) {
                auto color = material["pbrMetallicRoughness"]["baseColorFactor"].GetArray();
                curMaterial.color = Color{
                    color[0].GetFloat(),
                    color[1].GetFloat(),
                    color[2].GetFloat()
                };
                curMaterial.alpha = color[3].GetFloat();
            }
            if (material["pbrMetallicRoughness"].HasMember("metallicFactor")) {
                curMaterial.metallicFactor = material["pbrMetallicRoughness"]["metallicFactor"].GetFloat();
            }
        }
        if (material.HasMember("emissiveFactor")) {
            const auto emission = material["emissiveFactor"].GetArray();
            curMaterial.emission = Color{
                emission[0].GetFloat(),
                emission[1].GetFloat(),
                emission[2].GetFloat()
            };
        }
        if (material.HasMember("extensions") && material["extensions"].HasMember("KHR_materials_emissive_strength")) {
            float emissionFactor = material["extensions"]["KHR_materials_emissive_strength"]["emissiveStrength"].GetFloat();
            curMaterial.emission = emissionFactor * curMaterial.emission;
        }
        if (curMaterial.alpha < 1) {
            curMaterial.material = Material::DIELECTRIC;
        } else if (curMaterial.metallicFactor > 0) {
            curMaterial.material = Material::METALLIC;
        }
        scene.materials.push_back(curMaterial);
    }
}

void loadFigures(size_t indicesIndex, const Transition &transition, size_t material, const std::vector<Vec3> &positions, Scene &scene) {
    std::vector<Figure> figures;
    const auto &accessor = scene.accessors[indicesIndex];
    const auto &bufferView = scene.bufferViews[accessor.bufferView];
    const auto &buffer = scene.buffers[bufferView.buffer];
    if (accessor.type != "SCALAR") {
        std::cerr << "Load figures accessor: " << accessor.type << std::endl;
    }
    auto materialValue = scene.materials[material];
    assert(accessor.count % 3 == 0);
    for (size_t i = 0; i < accessor.count; i += 3) {
        size_t pos1, pos2, pos3;
        if (accessor.componentType == 5123) {
            pos1 = *(reinterpret_cast<const uint16_t*>(buffer.data() + bufferView.byteOffset + 2 * i));
            pos2 = *(reinterpret_cast<const uint16_t*>(buffer.data() + bufferView.byteOffset + 2 * (i + 1)));
            pos3 = *(reinterpret_cast<const uint16_t*>(buffer.data() + bufferView.byteOffset + 2 * (i + 2)));
        } else {
            pos1 = *(reinterpret_cast<const uint32_t*>(buffer.data() + bufferView.byteOffset + 4 * i));
            pos2 = *(reinterpret_cast<const uint32_t*>(buffer.data() + bufferView.byteOffset + 4 * (i + 1)));
            pos3 = *(reinterpret_cast<const uint32_t*>(buffer.data() + bufferView.byteOffset + 4 * (i + 2)));
        }
        Vec3 p1 = transition.apply(positions[pos1]);
        Vec3 p2 = transition.apply(positions[pos2]);
        Vec3 p3 = transition.apply(positions[pos3]);
        Figure fig(FigureType::TRIANGLE, p1, p2, p3);
        fig.color = materialValue.color;
        fig.emission = materialValue.emission;
        fig.material = materialValue.material;
        fig.ior = materialValue.ior;
        scene.figures.push_back(fig);
    }
}

void loadFiguresFromNodes(Scene &scene) {
    for (const auto &node : scene.nodes) {
        if (!node.mesh.has_value()) {
            continue;
        }
        const auto &mesh = node.mesh.value();
        for (const auto &primitive : scene.meshes[mesh].primitives) {
            auto positions = loadPositions(primitive.positions, scene);
            loadFigures(primitive.indices, node.totalTransition, primitive.material, positions, scene);
        }
    }
}

void loadCameraPosition(const rapidjson::Document &gltfScene, Scene &scene) {
    scene.cameraUp = {0, 1, 0};
    scene.cameraForward = {0, 0, -1};
    scene.cameraRight = {1, 0, 0};

    for (const auto &node : scene.nodes) {
        if (node.camera.has_value()) {
            scene.cameraFovY = gltfScene["cameras"].GetArray()[node.camera.value()]["perspective"]["yfov"].GetFloat();
            scene.cameraPos = node.totalTransition.apply({0, 0, 0});
            scene.cameraUp = node.totalTransition.apply(scene.cameraUp) - scene.cameraPos;
            scene.cameraRight = node.totalTransition.apply(scene.cameraRight) - scene.cameraPos;
            scene.cameraForward = node.totalTransition.apply(scene.cameraForward) - scene.cameraPos;
        }
    }
}

Scene loadScene(std::string_view gltfFilename) {
    Scene scene;

    std::ifstream in(gltfFilename.data(), std::ios_base::binary);
    rapidjson::IStreamWrapper isw(in);
    rapidjson::Document gltfScene;
    gltfScene.ParseStream(isw);

    loadBuffers(gltfFilename, gltfScene, scene);
    loadBufferViews(gltfScene, scene);
    loadNodes(gltfScene, scene);
    restoreNodeParents(scene);
    calculateTransitions(scene);
    loadMeshes(gltfScene, scene);
    loadAccessors(gltfScene, scene);
    loadMaterials(gltfScene, scene);
    loadFiguresFromNodes(scene);
    loadCameraPosition(gltfScene, scene);

    scene.initBVH();
    scene.initDistribution();
    return scene;
}

void renderScene(Scene &scene, std::string_view outFileName) {
    std::ofstream out(outFileName.data(), std::ios::binary);
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