#pragma once

#include "Input/WowFile.hpp"
#include "utility/Include/MathHelper.hpp"
#include "utility/Include/LinearAlgebra.hpp"

#include <string>
#include <algorithm>
#include <vector>

namespace parser
{
namespace input
{
struct DoodadInfo
{
    unsigned int NameId;
    unsigned int UniqueId;
    utility::Vertex BasePosition;
    float OrientationA;
    float OrientationB;
    float OrientationC;
    unsigned short Scale;
    unsigned short Flags;
};

class DoodadFile : WowFile
{
    private:
        static const unsigned int Magic = '02DM';

    public:
        std::vector<utility::Vertex> Vertices;
        std::vector<unsigned short> Indices;

        DoodadFile(const std::string &path);

        static std::string GetRealModelPath(const std::string &path);
};

class Doodad : public DoodadFile
{
    public:
        float MinZ;
        float MaxZ;

        Doodad(const std::string &path, const DoodadInfo &doodadInfo);
};
}
}