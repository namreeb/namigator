#pragma once

#include "utility/Include/LinearAlgebra.hpp"

#include <vector>
#include <string>
#include <set>

namespace parser
{
class Doodad
{
    private:
        void AmmendAdtSet(const std::vector<utility::Vertex> &vertices);

    public:
        std::vector<utility::Vertex> Vertices;
        std::vector<int> Indices;

        std::set<std::pair<int, int>> Adts;

        const float MinZ;
        const float MaxZ;

        Doodad(std::vector<utility::Vertex> &vertices, std::vector<unsigned short> &indices, const float minZ, const float maxZ);
};
}