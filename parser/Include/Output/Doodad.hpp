#pragma once

#include "utility/Include/LinearAlgebra.hpp"

#include <vector>
#include <string>

namespace parser
{
class Doodad
{
    public:
        std::vector<utility::Vertex> Vertices;
        std::vector<int> Indices;

        const float MinZ;
        const float MaxZ;

        Doodad(std::vector<utility::Vertex> &vertices, std::vector<unsigned short> &indices, const float minZ, const float maxZ);
};
}