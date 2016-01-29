#pragma once

#include "parser/Include/Map/Map.hpp"
#include "parser/Include/Wmo/WmoDoodad.hpp"

#include "utility/Include/LinearAlgebra.hpp"

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
        const std::string FullPath;
        std::string FileName;

        std::vector<utility::Vertex> Vertices;
        std::vector<std::int32_t> Indices;

        std::vector<utility::Vertex> LiquidVertices;
        std::vector<std::int32_t> LiquidIndices;

        std::vector<std::vector<std::unique_ptr<const WmoDoodad>>> DoodadSets;

        Wmo(Map *map, const std::string &path);

#ifdef _DEBUG
        size_t MemoryUsage() const;
#endif
};
}