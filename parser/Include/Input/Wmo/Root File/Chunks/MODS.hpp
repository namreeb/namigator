#pragma once

#include <vector>

#include "Input/Wmo/Root File/WmoRootChunk.hpp"

namespace parser_input
{
    struct DoodadSetInfo
    {
        char Name[20];
        unsigned int FirstDoodadIndex;
        unsigned int DoodadCount;
        unsigned int _unknown;
    };

    class MODS : WmoRootChunk
    {
        public:
            std::vector<DoodadSetInfo> DoodadSets;
            unsigned int Count;

            MODS(unsigned int doodadSetsCount, long position, utility::BinaryStream *reader);
    };
}