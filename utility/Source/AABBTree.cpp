#include "AABBTree.hpp"

#include <algorithm>

namespace utility
{
namespace {
class ModelFaceSorter {
    public:
    ModelFaceSorter(const ModelFace* faces, unsigned int numFaces, unsigned int axis)
        : faces(faces)
        , numFaces(numFaces)
        , axis(axis) {
    }

    bool operator () (unsigned int lhs, unsigned int rhs) const {
        float a = getCenteroid(lhs);
        float b = getCenteroid(rhs);
        return (a != b) ? (a < b) : (lhs < rhs);
    }

    float getCenteroid(unsigned int face) const {
        auto& a = faces[face].vertices[0];
        auto& b = faces[face].vertices[1];
        auto& c = faces[face].vertices[2];
        return (a[axis] + b[axis] + c[axis]) / 3.0f;
    }

    private:
    const ModelFace* faces;
    unsigned int numFaces;
    unsigned int axis;
};
}

AABBTree::AABBTree(const std::vector<ModelFace>& faces) {
    build(faces);
}

BoundingBox AABBTree::getBoundingBox() const {
    if (nodes.empty()) {
        return BoundingBox{};
    }
    return nodes.front().bounds;
}

BoundingBox AABBTree::calculateFaceBounds(unsigned int* faces, unsigned int numFaces) const {
    float min = std::numeric_limits<float>::lowest();
    float max = std::numeric_limits<float>::max();

    Vector3 minExtents = { max, max, max };
    Vector3 maxExtents = { min, min, min };

    for (unsigned int i = 0; i < numFaces; ++i) {
        auto& face = modelFaces.at(faces[i]);
        minExtents = utility::takeMinimum(minExtents, face.vertices[0]);
        maxExtents = utility::takeMaximum(maxExtents, face.vertices[0]);

        minExtents = utility::takeMinimum(minExtents, face.vertices[1]);
        maxExtents = utility::takeMaximum(maxExtents, face.vertices[1]);

        minExtents = utility::takeMinimum(minExtents, face.vertices[2]);
        maxExtents = utility::takeMaximum(maxExtents, face.vertices[2]);
    }

    return BoundingBox(minExtents, maxExtents);
}

void AABBTree::build(const std::vector<ModelFace>& newFaces) {
    modelFaces = newFaces;
    faceBounds.clear();
    faceIndices.clear();

    faceBounds.reserve(modelFaces.size());
    faceIndices.reserve(modelFaces.size());

    for (size_t i = 0; i < modelFaces.size(); ++i) {
        faceIndices.push_back(i);
        faceBounds.push_back(calculateFaceBounds(&i, 1));
    }

    freeNode = 1;
    nodes.clear();
    nodes.reserve(int(modelFaces.size() * 1.5f));

    buildRecursive(0, faceIndices.data(), modelFaces.size());
    faceBounds.clear();
}

unsigned int AABBTree::getLongestAxis(const Vector3& v) {
    if (v.X > v.Y && v.X > v.Z) {
        return 0;
    }
    return (v.Y > v.Z) ? 1 : 2;
}

unsigned int AABBTree::partitionMedian(Node& node, unsigned int* faces, unsigned int numFaces) {
    unsigned int axis = getLongestAxis(node.bounds.getVector());
    ModelFaceSorter predicate(modelFaces.data(), modelFaces.size(), axis);

    std::nth_element(faces, faces + numFaces / 2, faces + numFaces, predicate);
    return numFaces / 2;
}

unsigned int AABBTree::partitionSurfaceArea(Node& node, unsigned int* faces, unsigned int numFaces) {
    unsigned int bestAxis = 0;
    unsigned int bestIndex = 0;

    float bestCost = std::numeric_limits<float>::max();

    for (unsigned int axis = 0; axis < 3; ++axis) {
        ModelFaceSorter predicate(modelFaces.data(), modelFaces.size(), axis);
        std::sort(faces, faces + numFaces, predicate);

        // Two passes over data to calculate upper and lower bounds
        std::vector<float> cumulativeLower(numFaces);
        std::vector<float> cumulativeUpper(numFaces);

        BoundingBox lower, upper;

        for (unsigned int i = 0; i < numFaces; ++i) {
            lower.connectWith(faceBounds[faces[i]]);
            upper.connectWith(faceBounds[faces[numFaces - i - 1]]);

            cumulativeLower[i] = lower.getSurfaceArea();
            cumulativeUpper[numFaces - i - 1] = upper.getSurfaceArea();
        }

        float invTotalArea = 1.0f / cumulativeUpper[0];

        // Test split positions
        for (unsigned int i = 0; i < numFaces - 1; ++i) {
            float below = cumulativeLower[i] * invTotalArea;
            float above = cumulativeUpper[i] * invTotalArea;

            float cost = 0.125f + (below * i + above * (numFaces - i));
            if (cost <= bestCost) {
                bestCost = cost;
                bestIndex = i;
                bestAxis = axis;
            }
        }
    }

    // Sort faces by best axis
    ModelFaceSorter predicate(modelFaces.data(), modelFaces.size(), bestAxis);
    std::sort(faces, faces + numFaces, predicate);

    return bestIndex + 1;
}

void AABBTree::buildRecursive(unsigned int nodeIndex, unsigned int* faces, unsigned int numFaces) {
    const unsigned int maxFacesPerLeaf = 6;

    // Allocate more nodes if out of nodes
    if (nodeIndex >= nodes.size()) {
        int size = std::max(int(1.5f * nodes.size()), 512);
        nodes.resize(size);
    }

    auto& node = nodes[nodeIndex];
    node.bounds = calculateFaceBounds(faces, numFaces);

    if (numFaces <= maxFacesPerLeaf) {
        node.faces = faces;
        node.numFaces = numFaces;
    }
    else {
        unsigned int leftCount = partitionSurfaceArea(node, faces, numFaces);
        unsigned int rightCount = numFaces - leftCount;

        // Allocate 2 nodes
        node.children = freeNode;
        freeNode += 2;

        // Split faces in half and build each side recursively
        buildRecursive(node.children + 0, faces, leftCount);
        buildRecursive(node.children + 1, faces + leftCount, rightCount);
    }
}

bool AABBTree::intersectRay(Ray& ray, unsigned int* faceIndex) const {
    float distance = ray.getDistance();
    traceRecursive(0, ray, faceIndex);
    return ray.getDistance() < distance;
}

void AABBTree::traceRecursive(unsigned int nodeIndex, Ray& ray, unsigned int* faceIndex) const {
    auto& node = nodes.at(nodeIndex);
    if (node.faces != nullptr) {
        traceLeafNode(node, ray, faceIndex);
    }
    else {
        traceInnerNode(node, ray, faceIndex);
    }
}

void AABBTree::traceLeafNode(const Node& node, Ray& ray, unsigned int* faceIndex) const {
    float distance;

    for (unsigned int i = 0; i < node.numFaces; ++i) {
        auto verts = modelFaces.at(node.faces[i]).vertices;
        if (!ray.intersectTriangle(verts, &distance)) {
            continue;
        }

        if (distance < ray.getDistance()) {
            ray.setHitPoint(distance);

            if (faceIndex) {
                *faceIndex = node.faces[i];
            }
        }
    }
}

void AABBTree::traceInnerNode(const Node& node, Ray& ray, unsigned int* faceIndex) const {
    auto& leftChild = nodes.at(node.children + 0);
    auto& rightChild = nodes.at(node.children + 1);

    float max = std::numeric_limits<float>::max();
    float distance[2] = { max, max };

    ray.intersectBoundingBox(leftChild.bounds, &distance[0]);
    ray.intersectBoundingBox(rightChild.bounds, &distance[1]);

    unsigned int closest = 0;
    unsigned int furthest = 1;

    if (distance[1] < distance[0]) {
        std::swap(closest, furthest);
    }

    if (distance[closest] < ray.getDistance()) {
        traceRecursive(node.children + closest, ray, faceIndex);
    }

    if (distance[furthest] < ray.getDistance()) {
        traceRecursive(node.children + furthest, ray, faceIndex);
    }
}
}