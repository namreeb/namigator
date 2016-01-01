#pragma once

#include <vector>
#include <string>

#include "LinearAlgebra.hpp"
#include "BoundingBox.hpp"
#include "MathHelper.hpp"
#include "Input/WowFile.hpp"
#include "Input/Wmo/WmoParserInfo.hpp"

namespace parser_input
{
    class WmoRootFile : WowFile
    {
        public:
            std::vector<Vertex> Vertices;
            std::vector<int> Indices;

            std::vector<Vertex> LiquidVertices;
            std::vector<int> LiquidIndices;

            std::vector<Vertex> DoodadVertices;
            std::vector<int> DoodadIndices;

            BoundingBox Bounds;

            std::string Name;
            const std::string FullName;

            WmoRootFile(const std::string &path, const WmoParserInfo *info);
    };
}