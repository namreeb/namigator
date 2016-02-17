#include "AABBTree.hpp"

#include <fstream>
#include <algorithm>
#include <cstdint>
#include <cassert>

namespace utility
{
namespace {
class ModelFaceSorter {
    public:
        ModelFaceSorter(const Vertex* vertices, const int* indices, unsigned int axis)
            : m_vertices(vertices)
            , m_indices(indices)
            , m_axis(axis)
        {
        }

        bool operator () (unsigned int lhs, unsigned int rhs) const
        {
            float a = getCenteroid(lhs);
            float b = getCenteroid(rhs);
            return (a != b) ? (a < b) : (lhs < rhs);
        }

        float getCenteroid(unsigned int face) const
        {
            auto& a = m_vertices[m_indices[face*3 + 0]];
            auto& b = m_vertices[m_indices[face*3 + 1]];
            auto& c = m_vertices[m_indices[face*3 + 2]];
            return (a[m_axis] + b[m_axis] + c[m_axis]) / 3.0f;
        }

        private:
        const Vertex* m_vertices;
        const int* m_indices;
        unsigned int m_axis;
};
}

AABBTree::AABBTree(const std::vector<Vertex>& vertices, const std::vector<int>& indices)
{
    Build(vertices, indices);
}

BoundingBox AABBTree::GetBoundingBox() const
{
    if (m_nodes.empty())
        return BoundingBox{};
    return m_nodes.front().bounds;
}

void AABBTree::Serialize(std::ostream& stream) const
{
    const std::uint32_t magic = 'BVH1';
    stream.write(reinterpret_cast<const char *>(&magic), sizeof(magic));

    std::uint32_t size = static_cast<std::uint32_t>(m_vertices.size());
    stream.write(reinterpret_cast<const char *>(&size), sizeof(size));
    for (const auto& vertex : m_vertices)
        stream << vertex;

    size = static_cast<std::uint32_t>(m_indices.size());
    stream.write(reinterpret_cast<const char *>(&size), sizeof(size));
    for (const auto& index : m_indices)
    {
        const std::int32_t idx = index;
        stream.write(reinterpret_cast<const char *>(&idx), sizeof(idx));
    }

    size = static_cast<std::uint32_t>(m_nodes.size());
    stream.write(reinterpret_cast<const char *>(&size), sizeof(size));
    for (const auto& node : m_nodes)
    {
        assert(node.numFaces < 8);

        const std::uint8_t numFaces = static_cast<std::uint8_t>(node.numFaces);
        stream.write(reinterpret_cast<const char *>(&numFaces), sizeof(numFaces));
        if (!!node.numFaces)
        {
            // Write leaf node
            stream.write(reinterpret_cast<const char *>(&node.startFace), sizeof(node.startFace));
        }
        else
        {
            // Write inner node
            stream.write(reinterpret_cast<const char *>(&node.children), sizeof(node.children));
            stream << node.bounds;
        }
    }

    const std::uint32_t endMagic = 'FOOB';
    stream.write(reinterpret_cast<const char *>(&endMagic), sizeof(endMagic));
}

bool AABBTree::Deserialize(std::istream& stream)
{
    std::uint32_t magic;
    stream.read(reinterpret_cast<char *>(&magic), sizeof(magic));
    if (magic != (std::uint32_t)'BVH1')
        return false;

    std::uint32_t vertexCount;
    stream.read(reinterpret_cast<char *>(&vertexCount), sizeof(vertexCount));

    m_vertices.resize(vertexCount);
    stream.read(reinterpret_cast<char *>(&m_vertices[0]), vertexCount * sizeof(utility::Vertex));

    std::uint32_t indexCount;
    stream.read(reinterpret_cast<char *>(&indexCount), sizeof(indexCount));

    m_indices.reserve(indexCount);
    for (std::uint32_t i = 0; i < indexCount; ++i)
    {
        std::int32_t index;
        stream.read(reinterpret_cast<char *>(&index), sizeof(index));
        m_indices.push_back(static_cast<int>(index));
    }

    std::uint32_t nodeCount;
    stream.read(reinterpret_cast<char *>(&nodeCount), sizeof(nodeCount));
    m_nodes.resize(nodeCount);
    for (std::uint32_t i = 0; i < nodeCount; ++i)
    {
        std::uint8_t numFaces;
        stream.read(reinterpret_cast<char *>(&numFaces), sizeof(numFaces));

        m_nodes[i].numFaces = static_cast<unsigned int>(numFaces);
        if (!!numFaces)
        {
            // Read leaf node
            stream.read(reinterpret_cast<char *>(&m_nodes[i].startFace), sizeof(m_nodes[i].startFace));
        }
        else
        {
            // Read inner node
            stream.read(reinterpret_cast<char *>(&m_nodes[i].children), sizeof(m_nodes[i].children));
            stream >> m_nodes[i].bounds;
        }
    }

    std::uint32_t endMagic;
    stream.read(reinterpret_cast<char *>(&endMagic), sizeof(endMagic));

    return true;
}

BoundingBox AABBTree::CalculateFaceBounds(unsigned int* faces, unsigned int numFaces) const
{
    float min = std::numeric_limits<float>::lowest();
    float max = std::numeric_limits<float>::max();

    Vector3 minExtents = { max, max, max };
    Vector3 maxExtents = { min, min, min };

    for (unsigned int i = 0; i < numFaces; ++i)
    {
        auto& v0 = m_vertices[m_indices[faces[i] * 3 + 0]];
        auto& v1 = m_vertices[m_indices[faces[i] * 3 + 1]];
        auto& v2 = m_vertices[m_indices[faces[i] * 3 + 2]];

        minExtents = utility::takeMinimum(minExtents, v0);
        maxExtents = utility::takeMaximum(maxExtents, v0);

        minExtents = utility::takeMinimum(minExtents, v1);
        maxExtents = utility::takeMaximum(maxExtents, v1);

        minExtents = utility::takeMinimum(minExtents, v2);
        maxExtents = utility::takeMaximum(maxExtents, v2);
    }

    return BoundingBox(minExtents, maxExtents);
}

void AABBTree::Build(const std::vector<Vector3>& verts, const std::vector<int>& indices)
{
    m_vertices = verts;
    m_indices = indices;

    m_faceBounds.clear();
    m_faceIndices.clear();

    size_t numFaces = indices.size() / 3;

    m_faceBounds.reserve(numFaces);
    m_faceIndices.reserve(numFaces);

    for (unsigned int i = 0; i < unsigned int(numFaces); ++i)
    {
        m_faceIndices.push_back(i);
        m_faceBounds.push_back(CalculateFaceBounds(&i, 1));
    }

    m_freeNode = 1;
    m_nodes.clear();
    m_nodes.reserve(int(numFaces * 1.5f));

    BuildRecursive(0, m_faceIndices.data(), unsigned int(numFaces));
    m_faceBounds.clear();

    // Reorder the model indices according to the face indices
    std::vector<int> sortedIndices(m_indices.size());
    for (size_t i = 0; i < numFaces; ++i)
    {
        unsigned int index = m_faceIndices[i] * 3;
        sortedIndices[i*3 + 0] = m_indices[index + 0];
        sortedIndices[i*3 + 1] = m_indices[index + 1];
        sortedIndices[i*3 + 2] = m_indices[index + 2];
    }

    m_indices.swap(sortedIndices);
    m_faceIndices.clear();
}

unsigned int AABBTree::GetLongestAxis(const Vector3& v)
{
    if (v.X > v.Y && v.X > v.Z)
        return 0;

    return (v.Y > v.Z) ? 1 : 2;
}

unsigned int AABBTree::PartitionMedian(Node& node, unsigned int* faces, unsigned int numFaces)
{
    unsigned int axis = GetLongestAxis(node.bounds.getVector());
    ModelFaceSorter predicate(m_vertices.data(), m_indices.data(), axis);

    std::nth_element(faces, faces + numFaces / 2, faces + numFaces, predicate);
    return numFaces / 2;
}

unsigned int AABBTree::PartitionSurfaceArea(Node& /*node*/, unsigned int* faces, unsigned int numFaces)
{
    unsigned int bestAxis = 0;
    unsigned int bestIndex = 0;

    float bestCost = std::numeric_limits<float>::max();

    for (unsigned int axis = 0; axis < 3; ++axis)
    {
        ModelFaceSorter predicate(m_vertices.data(), m_indices.data(), axis);
        std::sort(faces, faces + numFaces, predicate);

        // Two passes over data to calculate upper and lower bounds
        std::vector<float> cumulativeLower(numFaces);
        std::vector<float> cumulativeUpper(numFaces);

        BoundingBox lower, upper;

        for (unsigned int i = 0; i < numFaces; ++i)
        {
            lower.connectWith(m_faceBounds[faces[i]]);
            upper.connectWith(m_faceBounds[faces[numFaces - i - 1]]);

            cumulativeLower[i] = lower.getSurfaceArea();
            cumulativeUpper[numFaces - i - 1] = upper.getSurfaceArea();
        }

        float invTotalArea = 1.0f / cumulativeUpper[0];

        // Test split positions
        for (unsigned int i = 0; i < numFaces - 1; ++i)
        {
            float below = cumulativeLower[i] * invTotalArea;
            float above = cumulativeUpper[i] * invTotalArea;

            float cost = 0.125f + (below * i + above * (numFaces - i));
            if (cost <= bestCost)
            {
                bestCost = cost;
                bestIndex = i;
                bestAxis = axis;
            }
        }
    }

    // Sort faces by best axis
    ModelFaceSorter predicate(m_vertices.data(), m_indices.data(), bestAxis);
    std::sort(faces, faces + numFaces, predicate);

    return bestIndex + 1;
}

void AABBTree::BuildRecursive(unsigned int nodeIndex, unsigned int* faces, unsigned int numFaces)
{
    const unsigned int maxFacesPerLeaf = 6;

    // Allocate more nodes if out of nodes
    if (nodeIndex >= m_nodes.size())
    {
        int size = std::max(int(1.5f * m_nodes.size()), 512);
        m_nodes.resize(size);
    }

    auto& node = m_nodes[nodeIndex];
    node.bounds = CalculateFaceBounds(faces, numFaces);

    if (numFaces <= maxFacesPerLeaf)
    {
        node.startFace = static_cast<std::uint32_t>(faces - m_faceIndices.data());
        assert(node.startFace == static_cast<std::size_t>(faces - m_faceIndices.data()));   // verify no truncation
        node.numFaces = numFaces;
    }
    else
    {
        unsigned int leftCount = PartitionSurfaceArea(node, faces, numFaces);
        unsigned int rightCount = numFaces - leftCount;

        // Allocate 2 nodes
        node.children = m_freeNode;
        m_freeNode += 2;

        // Split faces in half and build each side recursively
        BuildRecursive(node.children + 0, faces, leftCount);
        BuildRecursive(node.children + 1, faces + leftCount, rightCount);
    }
}

bool AABBTree::IntersectRay(Ray& ray, unsigned int* faceIndex) const
{
    float distance = ray.GetDistance();
    TraceRecursive(0, ray, faceIndex);
    return ray.GetDistance() < distance;
}

void AABBTree::Trace(Ray& ray, unsigned int* faceIndex) const
{
    struct StackEntry
    {
        unsigned int node;
        float dist;
    };

    StackEntry stack[50];
    stack[0].node = 0;
    stack[0].dist = 0.0f;

    float max = std::numeric_limits<float>::max();

    unsigned int stackCount = 1;
    while (!!stackCount) {
        // Pop node from back
        StackEntry& e = stack[--stackCount];

        // Ignore if another node has already come closer
        if (e.dist >= ray.GetDistance())
            continue;

        const Node& node = m_nodes.at(e.node);
        if (!node.numFaces)
        {
            // Find closest node
            auto& leftChild = m_nodes.at(node.children + 0);
            auto& rightChild = m_nodes.at(node.children + 1);

            float dist[2] = { max, max };
            ray.IntersectBoundingBox(leftChild.bounds, &dist[0]);
            ray.IntersectBoundingBox(rightChild.bounds, &dist[1]);

            unsigned int closest = dist[1] < dist[0]; // 0 or 1
            unsigned int furthest = closest ^ 1;

            if (dist[furthest] < ray.GetDistance())
            {
                StackEntry& n = stack[stackCount++];
                n.node = node.children + furthest;
                n.dist = dist[furthest];
            }

            if (dist[closest] < ray.GetDistance())
            {
                StackEntry& n = stack[stackCount++];
                n.node = node.children + closest;
                n.dist = dist[closest];
            }
        }
        else
            TraceLeafNode(node, ray, faceIndex);
    }
}

void AABBTree::TraceRecursive(unsigned int nodeIndex, Ray& ray, unsigned int* faceIndex) const {
    auto& node = m_nodes.at(nodeIndex);
    if (!!node.numFaces)
        TraceLeafNode(node, ray, faceIndex);
    else
        TraceInnerNode(node, ray, faceIndex);
}

void AABBTree::TraceLeafNode(const Node& node, Ray& ray, unsigned int* faceIndex) const
{
    for (auto i = node.startFace; i < node.startFace + node.numFaces; ++i)
    {
        auto& v0 = m_vertices[m_indices[i*3 + 0]];
        auto& v1 = m_vertices[m_indices[i*3 + 1]];
        auto& v2 = m_vertices[m_indices[i*3 + 2]];

        float distance;
        if (!ray.IntersectTriangle(v0, v1, v2, &distance))
            continue;

        if (distance < ray.GetDistance())
        {
            ray.SetHitPoint(distance);

            if (faceIndex)
                *faceIndex = i;
        }
    }
}

void AABBTree::TraceInnerNode(const Node& node, Ray& ray, unsigned int* faceIndex) const
{
    auto& leftChild = m_nodes.at(node.children + 0);
    auto& rightChild = m_nodes.at(node.children + 1);

    float max = std::numeric_limits<float>::max();
    float distance[2] = { max, max };

    ray.IntersectBoundingBox(leftChild.bounds, &distance[0]);
    ray.IntersectBoundingBox(rightChild.bounds, &distance[1]);

    unsigned int closest = 0;
    unsigned int furthest = 1;

    if (distance[1] < distance[0])
        std::swap(closest, furthest);

    if (distance[closest] < ray.GetDistance())
        TraceRecursive(node.children + closest, ray, faceIndex);

    if (distance[furthest] < ray.GetDistance())
        TraceRecursive(node.children + furthest, ray, faceIndex);
}
}