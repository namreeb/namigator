#pragma once

#include "utility/Include/LinearAlgebra.hpp"

#include <string>
#include <vector>

namespace parser
{
class Doodad
{
    private:
        static constexpr unsigned int Magic = '02DM';

    public:
        std::string FileName;

        std::vector<utility::Vertex> Vertices;
        std::vector<int> Indices;

        Doodad(const std::string &path);
};
}