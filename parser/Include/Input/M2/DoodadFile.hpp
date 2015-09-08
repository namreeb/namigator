#pragma once

#include <string>
#include <algorithm>
#include <vector>

#include "Input/WowFile.hpp"
#include "MathHelper.hpp"
#include "LinearAlgebra.hpp"

using namespace utility;

namespace parser_input
{
    struct DoodadInfo
    {
        unsigned int NameId;
        unsigned int UniqueId;
        Vertex BasePosition;
        float OrientationA;
        float OrientationB;
        float OrientationC;
        unsigned short Scale;
        unsigned short Flags;
    };

    class DoodadFile : WowFile
    {
        public:
            std::vector<Vertex> Vertices;
            std::vector<unsigned short> Indices;

            DoodadFile(const std::string &path);

            static std::string GetRealModelPath(const std::string &path);
    };

    class Doodad : public DoodadFile
    {
        public:
            float MinZ;
            float MaxZ;

            Doodad(const std::string &path, DoodadInfo doodadInfo);
    };

}