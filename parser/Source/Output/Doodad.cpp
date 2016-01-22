#include "parser.hpp"
#include "Output/Doodad.hpp"

#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/MathHelper.hpp"

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

    AmmendAdtSet(Vertices);
}

void Doodad::AmmendAdtSet(const std::vector<utility::Vertex> &vertices)
{
    for (auto const &v : vertices)
    {
        int adtX, adtY;
        utility::Convert::WorldToAdt(v, adtX, adtY);
        Adts.insert({ adtX, adtY });
    }
}
}