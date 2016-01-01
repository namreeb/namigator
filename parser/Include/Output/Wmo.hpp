#pragma once

#include <vector>
#include <string>

#include "LinearAlgebra.hpp"
#include "Output/Doodad.hpp"

using namespace utility;

namespace parser
{
    class Wmo
    {
        private:
            const float MinZ;
            const float MaxZ;

        public:
            std::vector<Vertex> Vertices;
            std::vector<int> Indices;

            std::vector<Vertex> LiquidVertices;
            std::vector<int> LiquidIndices;

            // we keep the vertex/index data here as opposed to a std::vector<Doodad *> because wmo doodads have no repeats / unique ids
            std::vector<Vertex> DoodadVertices;
            std::vector<int> DoodadIndices;

            Wmo(std::vector<Vertex> &vertices, std::vector<int> &indices, std::vector<Vertex> &liquidVertices,
                std::vector<int> &liquidIndices, std::vector<Vertex> &doodadVertices, std::vector<int> &doodadIndices,
                const float minZ, const float maxZ);
    };
}