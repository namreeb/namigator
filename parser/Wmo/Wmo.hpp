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
        const std::string MpqPath;

        std::vector<math::Vertex> Vertices;
        std::vector<std::int32_t> Indices;

        std::vector<math::Vertex> LiquidVertices;
        std::vector<std::int32_t> LiquidIndices;

        std::vector<std::vector<std::unique_ptr<const WmoDoodad>>> DoodadSets;

        Wmo(const std::string &path);
};
}