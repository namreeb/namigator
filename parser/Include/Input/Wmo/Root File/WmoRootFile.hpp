#pragma once

#include "Input/WowFile.hpp"
#include "Input/Wmo/WmoParserInfo.hpp"

#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/BoundingBox.hpp"
#include "utility/Include/MathHelper.hpp"

#include <vector>
#include <string>

namespace parser
{
namespace input
{
    class WmoRootFile : WowFile
    {
        public:
            std::vector<utility::Vertex> Vertices;
            std::vector<int> Indices;

            std::vector<utility::Vertex> LiquidVertices;
            std::vector<int> LiquidIndices;

            std::vector<utility::Vertex> DoodadVertices;
            std::vector<int> DoodadIndices;

            utility::BoundingBox Bounds;

            std::string Name;
            const std::string FullName;

            WmoRootFile(const std::string &path, const WmoParserInfo *info);
    };
}
}