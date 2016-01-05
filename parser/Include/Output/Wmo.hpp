#pragma once

#include "utility/Include/LinearAlgebra.hpp"
#include "parser/Include/Output/Doodad.hpp"

#include <vector>
#include <string>

namespace parser
{
class Wmo
{
    private:
        const float MinZ;
        const float MaxZ;

    public:
        std::vector<utility::Vertex> Vertices;
        std::vector<int> Indices;

        std::vector<utility::Vertex> LiquidVertices;
        std::vector<int> LiquidIndices;

        // we keep the vertex/index data here as opposed to a std::vector<Doodad *> because wmo doodads have no repeats / unique ids
        std::vector<utility::Vertex> DoodadVertices;
        std::vector<int> DoodadIndices;

        Wmo(std::vector<utility::Vertex> &vertices, std::vector<int> &indices,
            std::vector<utility::Vertex> &liquidVertices, std::vector<int> &liquidIndices,
            std::vector<utility::Vertex> &doodadVertices, std::vector<int> &doodadIndices,
            const float minZ, const float maxZ);
};
}