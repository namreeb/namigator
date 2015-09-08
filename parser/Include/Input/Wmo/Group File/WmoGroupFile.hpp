#pragma once

#include <string>

#include "Input/WowFile.hpp"
#include "Input/Wmo/Group File/Chunks/MOPY.hpp"
#include "Input/Wmo/Group File/Chunks/MOVI.hpp"
#include "Input/Wmo/Group File/Chunks/MOVT.hpp"
#include "Input/Wmo/Group File/Chunks/MLIQ.hpp"

namespace parser_input
{
    class WmoGroupFile : public WowFile
    {
        public:
            int Index;
            unsigned int Flags;

            MOPY *MaterialsChunk;
            MOVI *IndicesChunk;
            MOVT *VerticesChunk;
            MLIQ *LiquidChunk;

            // bounding box.  XXX - do we actually need this?
            float Min[3];
            float Max[3];

            WmoGroupFile(const std::string &path);
            ~WmoGroupFile();
    };
}