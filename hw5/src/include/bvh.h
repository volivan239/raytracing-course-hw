#pragma once

#include "vec3.h"
#include "primitives.h"
#include "quaternion.h"
#include <cassert>
#include <iostream>

class Node {
public:
    AABB aabb;
    uint32_t left = 0, right = 0, first, last;

    Node() {}
    Node(uint32_t first, uint32_t last): first(first), last(last) {}
};

class BVH {
public:
    std::vector<Node> nodes;
    uint32_t root; 
    mutable int counter = 0;

    BVH() {}
    BVH(std::vector<Figure> &figures, uint32_t n) {
        root = buildNode(figures, 0, n);
    }

    std::optional<std::pair<Intersection, int>> intersect(const std::vector<Figure> &figures, const Ray &ray, std::optional<float> curBest) const {
        return intersect_(figures, root, ray, curBest);
    }

private:
    std::pair<float, uint32_t> bestSplit(std::vector<Figure> &figures, uint32_t first, uint32_t last) const {
        std::vector<float> scores(last - first, 0);
        AABB prefixAABB(figures[first]);
        for (size_t i = 1; i < last - first; i++) {
            scores[i] = prefixAABB.getS() * i;
            prefixAABB.extend(figures[first + i]);
        }

        AABB suffixAABB(figures[last - 1]);
        for (size_t i = last - first - 1; i >= 1; i--) {
            scores[i] += suffixAABB.getS() * ((last - first) - i);
            suffixAABB.extend(figures[first + i - 1]);
        }
        std::pair<float, uint32_t> ans = {scores[1], first + 1};
        for (size_t i = 2; i < last - first; i++) {
            if (scores[i] < ans.first) {
                ans = {scores[i], first + i};
            }
        }
        return ans;
    }

    enum class Axis {
        X, Y, Z
    };

    void halfSplit(std::vector<Figure> &figures, uint32_t first, uint32_t last, Axis axis) const {
        uint32_t halfSize = (last - first) / 2;
        auto cmp = axis == Axis::X ? [](const Figure &lhs, const Figure &rhs) { return lhs.position.x < rhs.position.x; } : 
                  (axis == Axis::Y ? [](const Figure &lhs, const Figure &rhs) { return lhs.position.y < rhs.position.y; } : 
                                     [](const Figure &lhs, const Figure &rhs) { return lhs.position.z < rhs.position.z; });
        std::nth_element(figures.begin() + first, figures.begin() + halfSize, figures.begin() + last, cmp);
    }

    uint32_t buildNode(std::vector<Figure> &figures, uint32_t first, uint32_t last) {
        Node cur = Node(first, last);
        AABB aabb;
        if (first < last) {
            aabb = AABB(figures[first]);
        }
        for (uint32_t i = first + 1; i < last; i++) {
            aabb.extend(figures[i]);
        }
        cur.aabb = aabb;
        uint32_t thisPos = nodes.size();
        nodes.push_back(cur);
        if (last - first <= 1) {
            return thisPos;
        }

        halfSplit(figures, first, last, Axis::X);
        auto splitX = bestSplit(figures, first, last);
        halfSplit(figures, first, last, Axis::Y);
        auto splitY = bestSplit(figures, first, last);
        halfSplit(figures, first, last, Axis::Z);
        auto splitZ = bestSplit(figures, first, last);

        float bestResult = std::min(splitX.first, std::min(splitY.first, splitZ.first));
        if (bestResult >= aabb.getS() * (last - first)) {
            return thisPos;
        }
        
        uint32_t mid;
        if (bestResult == splitX.first) {
            mid = splitX.second;
            halfSplit(figures, first, last, Axis::X);
        } else if (bestResult == splitY.first) {
            mid = splitY.second;
            halfSplit(figures, first, last, Axis::Y);
        } else {
            mid = splitZ.second;
            halfSplit(figures, first, last, Axis::Z);
        }
        nodes[thisPos].left = buildNode(figures, first, mid);
        nodes[thisPos].right = buildNode(figures, mid, last);
        return thisPos;
    }

    std::optional<std::pair<Intersection, int>> intersect_(const std::vector<Figure> &figures, uint32_t pos, const Ray &ray, std::optional<float> curBest) const {
        const Node &cur = nodes[pos];

        std::optional<std::pair<Intersection, int>> bestIntersection = {};
        if (cur.left == 0) {
            for (uint32_t i = cur.first; i < cur.last; i++) {
                auto curIntersection = figures[i].intersect(ray);
                if (curIntersection.has_value() && (!bestIntersection.has_value() || curIntersection.value().t < bestIntersection.value().first.t)) {
                    bestIntersection = {curIntersection.value(), i};
                }
            }
            return bestIntersection;
        }

        auto leftIntersection0 = nodes[cur.left].aabb.intersect(ray);
        if (!leftIntersection0.has_value()) {
            return intersect_(figures, cur.right, ray, curBest);
        }
        auto [lt, _, lIsInside] = leftIntersection0.value();

        auto rightIntersection0 = nodes[cur.right].aabb.intersect(ray);
        if (!rightIntersection0.has_value()) {
            return intersect_(figures, cur.left, ray, curBest);
        }
        auto [rt, __, rIsInside] = rightIntersection0.value();

        int left = cur.left, right = cur.right;
        if (rIsInside || (!lIsInside && rt < lt)) {
            std::swap(left, right);
            std::swap(lt, rt);
            std::swap(lIsInside, rIsInside);
        }

        auto leftIntersection = (curBest.has_value() && !lIsInside && curBest.value() < lt) ? std::nullopt : intersect_(figures, left, ray, curBest);
        bestIntersection = leftIntersection;
        if (leftIntersection.has_value() && (!curBest.has_value() || leftIntersection.value().first.t < curBest.value())) {
            curBest = leftIntersection.value().first.t;
        }

        auto rightIntersection = (curBest.has_value() && !rIsInside && curBest.value() < rt) ? std::nullopt : intersect_(figures, right, ray, curBest);
        if (rightIntersection.has_value() && (!bestIntersection.has_value() || rightIntersection.value().first.t < bestIntersection.value().first.t)) {
            bestIntersection = rightIntersection;
        }
        return bestIntersection;
    }
};