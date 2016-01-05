#include "Output/Wmo.hpp"
#include "parser.hpp"

#include <string>
#include <fstream>

namespace parser
{
Wmo::Wmo(std::vector<utility::Vertex> &vertices, std::vector<int> &indices,
         std::vector<utility::Vertex> &liquidVertices, std::vector<int> &liquidIndices,
         std::vector<utility::Vertex> &doodadVertices, std::vector<int> &doodadIndices,
         const float minZ, const float maxZ)
    : MinZ(minZ), MaxZ(maxZ),
      Vertices(std::move(vertices)), Indices(std::move(indices)),
      LiquidVertices(std::move(liquidVertices)), LiquidIndices(std::move(liquidIndices)),
      DoodadVertices(std::move(doodadVertices)), DoodadIndices(std::move(doodadIndices)) {}
}