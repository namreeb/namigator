#pragma once

#include "BinaryStream.hpp"
#include "BoundingBox.hpp"
#include "Ray.hpp"
#include "Vector.hpp"

#include <cstdint>
#include <vector>

namespace math
{
class AABBTree
{
private:
    struct Node
    {
        union
        {
            std::uint32_t children = 0;
            std::uint32_t startFace;
        };

        unsigned int numFaces = 0;
        BoundingBox bounds;
    };

    static constexpr std::uint32_t StartMagic = 'BVH1';
    static constexpr std::uint32_t EndMagic = 'FOOB';

public:
    AABBTree() = default;
    AABBTree(AABBTree&& other) = default;
    ~AABBTree() = default;

    AABBTree(const std::vector<Vertex>& verts, const std::vector<int>& indices);

    AABBTree& operator=(AABBTree&& other) = default;

public:
    void Build(const std::vector<Vertex>& verts,
               const std::vector<int>& indices);
    bool IntersectRay(Ray& ray, unsigned int* faceIndex = nullptr) const;

    BoundingBox GetBoundingBox() const;

    void Serialize(utility::BinaryStream& stream) const;
    bool Deserialize(utility::BinaryStream& stream);

    const std::vector<Vector3>& Vertices() const { return m_vertices; }
    const std::vector<int>& Indices() const { return m_indices; }

private:
    unsigned int PartitionMedian(Node& node, unsigned int* faces,
                                 unsigned int numFaces);
    unsigned int PartitionSurfaceArea(Node& node, unsigned int* faces,
                                      unsigned int numFaces);

    void BuildRecursive(unsigned int nodeIndex, unsigned int* faces,
                        unsigned int numFaces);
    BoundingBox CalculateFaceBounds(unsigned int* faces,
                                    unsigned int numFaces) const;

    void Trace(Ray& ray, unsigned int* faceIndex) const;
    void TraceRecursive(unsigned int nodeIndex, Ray& ray,
                        unsigned int* faceIndex) const;
    void TraceInnerNode(const Node& node, Ray& ray,
                        unsigned int* faceIndex) const;
    void TraceLeafNode(const Node& node, Ray& ray,
                       unsigned int* faceIndex) const;

    static unsigned int GetLongestAxis(const Vector3& v);

private:
    unsigned int m_freeNode = 0;

    std::vector<Node> m_nodes;

    std::vector<Vertex> m_vertices;
    std::vector<int> m_indices;

    std::vector<BoundingBox> m_faceBounds;
    std::vector<unsigned int> m_faceIndices;
};
} // namespace math