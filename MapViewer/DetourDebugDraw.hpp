#pragma once

#include "Renderer.hpp"

#include "recastnavigation/DebugUtils/Include/DebugDraw.h"

#include "utility/Vector.hpp"

#include <unordered_map>
#include <vector>

namespace std
{
template <>
struct hash<math::Vertex>
{
    std::size_t operator()(const math::Vertex& vertex) const
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
        bool m_steep;

        std::unordered_map<math::Vertex, int> m_uniqueVertices;
        std::vector<math::Vertex> m_vertices;
        std::vector<int> m_indices;
        std::vector<unsigned int> m_colors;

        Renderer * const m_renderer;

    public:
        DetourDebugDraw(Renderer *renderer);

        void Steep(bool steep) { m_steep = steep; }

        virtual void depthMask(bool /*state*/) {}
        virtual void texture(bool /*state*/) {}
        virtual void begin(duDebugDrawPrimitives prim, float size = 1.f);
        virtual void vertex(const float* pos, unsigned int color);
        virtual void vertex(const float x, const float y, const float z, unsigned int color);
        virtual void vertex(const float* pos, unsigned int color, const float* uv);
        virtual void vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v);
        virtual void end();
};