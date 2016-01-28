#pragma once

#include "Ray.hpp"
#include "BinaryStream.hpp"
#include "LinearAlgebra.hpp"
#include "BoundingBox.hpp"

#include <fstream>
#include <vector>

namespace utility
{
class AABBTree
{
    private:
        struct Node
        {
            union
            {
                unsigned int children = 0;
                unsigned int startFace;
            };

            unsigned int numFaces = 0;
            BoundingBox bounds;
        };

    public:
        AABBTree() = default;
        ~AABBTree() = default;

        AABBTree(const std::vector<Vertex>& verts, const std::vector<int>& indices);

    public:
        void Build(const std::vector<Vertex>& verts, const std::vector<int>& indices);
        bool IntersectRay(Ray& ray, unsigned int* faceIndex = nullptr) const;

        BoundingBox GetBoundingBox() const;

        void Serialize(std::ostream& stream) const;
        bool Deserialize(std::istream& stream);

    private:
        unsigned int PartitionMedian(Node& node, unsigned int* faces, unsigned int numFaces);
        unsigned int PartitionSurfaceArea(Node& node, unsigned int* faces, unsigned int numFaces);

        void BuildRecursive(unsigned int nodeIndex, unsigned int* faces, unsigned int numFaces);
        BoundingBox CalculateFaceBounds(unsigned int* faces, unsigned int numFaces) const;

        void Trace(Ray& ray, unsigned int* faceIndex) const;
        void TraceRecursive(unsigned int nodeIndex, Ray& ray, unsigned int* faceIndex) const;
        void TraceInnerNode(const Node& node, Ray& ray, unsigned int* faceIndex) const;
        void TraceLeafNode(const Node& node, Ray& ray, unsigned int* faceIndex) const;

        static unsigned int GetLongestAxis(const Vector3& v);

    private:
        unsigned int m_freeNode = 0;

        std::vector<Node> m_nodes;
        std::vector<Vertex> m_vertices;
        std::vector<int> m_indices;

        std::vector<BoundingBox> m_faceBounds;
        std::vector<unsigned int> m_faceIndices;
};
}