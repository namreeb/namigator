#pragma once

#include "Ray.hpp"

#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/BoundingBox.hpp"

#include <vector>

namespace utility
{
struct ModelFace {
    Vector3 vertices[3];
};

class AABBTree {
    private:
    struct Node {
        union {
            unsigned int children = 0;
            unsigned int numFaces;
        };

        unsigned int* faces = nullptr;
        BoundingBox bounds;
    };

    public:
    AABBTree() = default;
    ~AABBTree() = default;

    AABBTree(const std::vector<ModelFace>& faces);

    public:
    void build(const std::vector<ModelFace>& faces);
    bool intersectRay(Ray& ray, unsigned int* faceIndex = nullptr) const;

    BoundingBox getBoundingBox() const;

    private:
    unsigned int partitionMedian(Node& node, unsigned int* faces, unsigned int numFaces);
    unsigned int partitionSurfaceArea(Node& node, unsigned int* faces, unsigned int numFaces);

    void buildRecursive(unsigned int nodeIndex, unsigned int* faces, unsigned int numFaces);
    BoundingBox calculateFaceBounds(unsigned int* faces, unsigned int numFaces) const;

    void traceRecursive(unsigned int nodeIndex, Ray& ray, unsigned int* faceIndex) const;
    void traceInnerNode(const Node& node, Ray& ray, unsigned int* faceIndex) const;
    void traceLeafNode(const Node& node, Ray& ray, unsigned int* faceIndex) const;

    static unsigned int getLongestAxis(const Vector3& v);

    private:
    unsigned int freeNode = 0;

    std::vector<Node> nodes;
    std::vector<ModelFace> modelFaces;
    std::vector<BoundingBox> faceBounds;
    std::vector<unsigned int> faceIndices;
};
}