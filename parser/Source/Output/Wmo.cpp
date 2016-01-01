#include <string>
#include <fstream>

#include "Output/Wmo.hpp"
#include "parser.hpp"

namespace parser
{
    Wmo::Wmo(std::vector<Vertex> &vertices, std::vector<int> &indices, std::vector<Vertex> &liquidVertices,
             std::vector<int> &liquidIndices, std::vector<Vertex> &doodadVertices, std::vector<int> &doodadIndices,
             const float minZ, const float maxZ) : MinZ(minZ), MaxZ(maxZ)
    {
        Vertices = std::move(vertices);
        Indices = std::move(indices);

        LiquidVertices = std::move(liquidVertices);
        LiquidIndices = std::move(liquidIndices);

        DoodadVertices = std::move(doodadVertices);
        DoodadIndices = std::move(doodadIndices);
    }
}