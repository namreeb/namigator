#pragma once

#include "parser/Wmo/WmoDoodad.hpp"

#include "utility/Vector.hpp"

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace parser
{
// This type refers to model-specifc information.  It does not contain information that has been translated into world space.  For that, see WmoInstance
class Wmo
{
    public:
        std::string FileName;

        std::vector<math::Vector3> Vertices;
        std::vector<std::int32_t> Indices;

        std::vector<math::Vector3> LiquidVertices;
        std::vector<std::int32_t> LiquidIndices;

        std::vector<std::vector<std::unique_ptr<const WmoDoodad>>> DoodadSets;

        Wmo(const std::string &path);
};
}