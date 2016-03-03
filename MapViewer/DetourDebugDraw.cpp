#include "DetourDebugDraw.hpp"

#include "utility/Include/MathHelper.hpp"

#include <vector>
#include <iostream>
#include <unordered_map>

#ifdef _DEBUG
#include <cassert>
#endif

void DetourDebugDraw::begin(duDebugDrawPrimitives prim, float size)
{
    m_type = prim;
    m_size = size;
    m_uniqueVertices.clear();
    m_vertices.clear();
    m_indices.clear();
}

void DetourDebugDraw::vertex(const float* pos, unsigned int /*color*/)
{
    utility::Vertex v;
    utility::Convert::VertexToWow(pos, v);

    if (m_uniqueVertices.find(v) == m_uniqueVertices.end())
    {
        m_uniqueVertices[v] = static_cast<int>(m_vertices.size());
        m_vertices.push_back(v);
    }

    m_indices.push_back(m_uniqueVertices[v]);
}

void DetourDebugDraw::vertex(const float x, const float y, const float z, unsigned int color)
{
    const float rv[] { x,y,z };
    utility::Vertex v;

    vertex(rv, color);
}

void DetourDebugDraw::vertex(const float* pos, unsigned int color, const float* /*uv*/)
{
    vertex(pos, color);
}

void DetourDebugDraw::vertex(const float x, const float y, const float z, unsigned int color, const float /*u*/, const float /*v*/)
{
    vertex(x, y, z, color);
}

void DetourDebugDraw::end()
{
    switch (m_type)
    {
        case duDebugDrawPrimitives::DU_DRAW_LINES:
            // slight kludge to ignore inner mesh poly lines for now
            if (m_size != IgnoreLineSize)
                m_renderer->AddLines(m_vertices, m_indices);

            break;

        case duDebugDrawPrimitives::DU_DRAW_TRIS:
            m_renderer->AddMesh(m_vertices, m_indices);
            break;

        default:
            break;
    }
}