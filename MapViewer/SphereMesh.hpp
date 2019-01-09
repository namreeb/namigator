#pragma once

#include "utility/MathHelper.hpp"

#include <vector>
#include <unordered_map>

// Taken from: http://blog.andreaskahler.com/2009/06/creating-icosphere-mesh-in-code.html

class SphereMesh
{
public:
    SphereMesh(int recursionLevel);
    ~SphereMesh() = default;

    std::vector<math::Vertex> const& GetVertices() const
    {
        return m_vertices;
    }

    std::vector<int> const& GetIndices() const
    {
        return m_indices;
    }

private:
    int AddVertex(math::Vertex const& v);
    int AddMiddlePoint(int p1, int p2);

    std::vector<math::Vertex> m_vertices;
    std::vector<int> m_indices;
    int m_index = 0;

    std::unordered_map<std::int64_t, int> m_middlePointIndexCache;
};

inline SphereMesh::SphereMesh(int recursionLevel)
{
    struct TriFace
    {
        int v1, v2, v3;
    };

    float t = (1.0f + std::sqrtf(5.0f)) / 2.0f;

    AddVertex(math::Vertex{ -1, t, 0 });
    AddVertex(math::Vertex{ 1, t, 0 });
    AddVertex(math::Vertex{ -1, -t, 0 });
    AddVertex(math::Vertex{ 1, -t, 0 });

    AddVertex(math::Vertex{ 0, -1, t });
    AddVertex(math::Vertex{ 0, 1, t });
    AddVertex(math::Vertex{ 0, -1, -t });
    AddVertex(math::Vertex{ 0, 1, -t });

    AddVertex(math::Vertex{ t, 0, -1 });
    AddVertex(math::Vertex{ t, 0, 1 });
    AddVertex(math::Vertex{ -t, 0, -1 });
    AddVertex(math::Vertex{ -t, 0, 1 });

    std::vector<TriFace> faces;
    faces.reserve(20);

    // 5 faces around point 0
    faces.push_back(TriFace{ 0, 11, 5 });
    faces.push_back(TriFace{ 0, 5, 1 });
    faces.push_back(TriFace{ 0, 1, 7 });
    faces.push_back(TriFace{ 0, 7, 10 });
    faces.push_back(TriFace{ 0, 10, 11 });

    // 5 adjacent faces 
    faces.push_back(TriFace{ 1, 5, 9 });
    faces.push_back(TriFace{ 5, 11, 4 });
    faces.push_back(TriFace{ 11, 10, 2 });
    faces.push_back(TriFace{ 10, 7, 6 });
    faces.push_back(TriFace{ 7, 1, 8 });

    // 5 faces around point 3
    faces.push_back(TriFace{ 3, 9, 4 });
    faces.push_back(TriFace{ 3, 4, 2 });
    faces.push_back(TriFace{ 3, 2, 6 });
    faces.push_back(TriFace{ 3, 6, 8 });
    faces.push_back(TriFace{ 3, 8, 9 });

    // 5 adjacent faces 
    faces.push_back(TriFace{ 4, 9, 5 });
    faces.push_back(TriFace{ 2, 4, 11 });
    faces.push_back(TriFace{ 6, 2, 10 });
    faces.push_back(TriFace{ 8, 6, 7 });
    faces.push_back(TriFace{ 9, 8, 1 });

    for (int i = 0; i < recursionLevel; ++i)
    {
        std::vector<TriFace> faces2;
        faces2.reserve(4 * faces.size());

        for (auto const& tri : faces)
        {
            int a = AddMiddlePoint(tri.v1, tri.v2);
            int b = AddMiddlePoint(tri.v2, tri.v3);
            int c = AddMiddlePoint(tri.v3, tri.v1);

            faces2.push_back(TriFace{ tri.v1, a, c });
            faces2.push_back(TriFace{ tri.v2, b, a });
            faces2.push_back(TriFace{ tri.v3, c, b });
            faces2.push_back(TriFace{ a, b, c });
        }
        faces.swap(faces2);
    }

    m_indices.reserve(3 * faces.size());

    for (auto const& tri : faces)
    {
        m_indices.push_back(tri.v1);
        m_indices.push_back(tri.v2);
        m_indices.push_back(tri.v3);
    }
}

inline int SphereMesh::AddVertex(math::Vertex const& v)
{
    m_vertices.push_back(math::Vertex::Normalize(v));
    return m_index++;
}

inline int SphereMesh::AddMiddlePoint(int p1, int p2)
{
    bool firstIsSmaller = p1 < p2;
    int64_t smallerIndex = firstIsSmaller ? p1 : p2;
    int64_t greaterIndex = firstIsSmaller ? p2 : p1;
    int64_t key = (smallerIndex << 32) + greaterIndex;

    auto const itr = m_middlePointIndexCache.find(key);
    if (itr != m_middlePointIndexCache.end())
    {
        return itr->second;
    }

    auto const point1 = m_vertices[p1];
    auto const point2 = m_vertices[p2];
    auto const middle = (point1 + point2) * 0.5f;

    int i = AddVertex(middle);

    m_middlePointIndexCache.emplace_hint(itr, key, i);
    return i;
}
