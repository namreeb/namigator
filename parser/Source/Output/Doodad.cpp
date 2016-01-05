#include "parser.hpp"
#include "utility/Include/LinearAlgebra.hpp"
#include "Output/Doodad.hpp"

#include <vector>
#include <string>
#include <fstream>

namespace parser
{
Doodad::Doodad(std::vector<utility::Vertex> &vertices, std::vector<unsigned short> &indices, const float minZ, const float maxZ)
    : MinZ(minZ), MaxZ(maxZ), Vertices(std::move(vertices)), Indices(indices.size())
{
    for (size_t i = 0; i < indices.size(); ++i)
        Indices[i] = indices[i];
}
}