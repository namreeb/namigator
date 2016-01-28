#pragma once

#include "Wmo/Root File/WmoRootChunk.hpp"

#include <vector>

namespace parser
{
namespace input
{
struct DoodadSetInfo
{
    public:
        char Name[20];
        unsigned int FirstDoodadIndex;
        unsigned int DoodadCount;
    private:
        unsigned int _unknown;
};

class MODS : WmoRootChunk
{
    public:
        const unsigned int Count;
        std::vector<DoodadSetInfo> DoodadSets;

        MODS(unsigned int doodadSetsCount, size_t position, utility::BinaryStream *reader);
};
}
}