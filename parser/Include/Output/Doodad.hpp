#pragma once

#include <vector>
#include <string>

#include "LinearAlgebra.hpp"

using namespace utility;

namespace parser
{
    class Doodad
    {
        public:
            std::vector<Vertex> Vertices;
            std::vector<unsigned short> Indices;

            const float MinZ;
            const float MaxZ;

            Doodad(std::vector<Vertex> &vertices, std::vector<unsigned short> &indices, const float minZ, const float maxZ);

            void SaveToDisk(const std::string &path) const;
    };
}