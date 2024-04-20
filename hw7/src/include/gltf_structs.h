#pragma once

#include <stddef.h>
#include <vector>
#include "transition.h"
#include "material.h"

using Buffer = std::vector<char>;

struct BufferView {
    size_t buffer;
    size_t byteLength;
    size_t byteOffset;
};

struct Node {
    std::optional<std::size_t> mesh = {};
    std::optional<std::size_t> camera = {};
    Quaternion rotation;
    Vec3 scale = {1, 1, 1};
    Vec3 translation;
    std::optional<Transition> transition = {};
    std::vector<size_t> children;
    std::optional<size_t> parentNode = {};
    Transition totalTransition;
};

struct GltfMaterial {
    Color color = {1, 1, 1};
    Color emission = {0, 0, 0};
    float metallicFactor = 1.0;
    float roughnessFactor = 1.0;

    static constexpr const float ior = 1.5;
};

struct Primitive {
    std::size_t positions;
    std::size_t normals;
    std::size_t indices;
    std::size_t material;
};

struct Mesh {
    std::vector<Primitive> primitives;
};

struct Accessor {
    std::size_t bufferView;
    std::size_t count;
    std::size_t componentType;
    std::string type;
    std::size_t byteOffset;
};