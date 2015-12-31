#include <vector>
#include <string>
#include <fstream>

#include "parser.hpp"
#include "LinearAlgebra.hpp"
#include "Output/Doodad.hpp"

namespace parser
{
    Doodad::Doodad(std::vector<Vertex> &vertices, std::vector<unsigned short> &indices, const float minZ, const float maxZ)
        : MinZ(minZ), MaxZ(maxZ), Vertices(std::move(vertices)), Indices(indices.size())
    {
        for (size_t i = 0; i < indices.size(); ++i)
            Indices[i] = indices[i];
    }
}