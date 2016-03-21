#pragma once

#include "Renderer.hpp"

#include "DebugUtils/Include/DebugDraw.h"

#include "utility/Include/LinearAlgebra.hpp"

#include <unordered_map>
#include <vector>

namespace std
{
template <>
struct hash<utility::Vertex>
{
    std::size_t operator()(const utility::Vertex& vertex) const
    {
        return std::hash<float>()(vertex.X) + std::hash<float>()(vertex.Y);
    }
};
}

class DetourDebugDraw : public duDebugDraw
{
    private:
        static constexpr float IgnoreLineSize = 1.5f;
        duDebugDrawPrimitives m_type;
        float m_size;

        std::unordered_map<utility::Vertex, int> m_uniqueVertices;
        std::vector<utility::Vertex> m_vertices;
        std::vector<int> m_indices;

        Renderer * const m_renderer;

    public:
        DetourDebugDraw(Renderer *renderer) : m_renderer(renderer), m_size(0.f) {}

        virtual void depthMask(bool /*state*/) {}
        virtual void texture(bool /*state*/) {}
        virtual void begin(duDebugDrawPrimitives prim, float size = 1.f);
        virtual void vertex(const float* pos, unsigned int color);
        virtual void vertex(const float x, const float y, const float z, unsigned int color);
        virtual void vertex(const float* pos, unsigned int color, const float* uv);
        virtual void vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v);
        virtual void end();
};