#pragma once

#include "Input/WowFile.hpp"
#include "Input/Wmo/Group File/Chunks/MOPY.hpp"
#include "Input/Wmo/Group File/Chunks/MOVI.hpp"
#include "Input/Wmo/Group File/Chunks/MOVT.hpp"
#include "Input/Wmo/Group File/Chunks/MLIQ.hpp"

#include <string>
#include <memory>

namespace parser
{
namespace input
{
class WmoGroupFile : public WowFile
{
    public:
        int Index;
        unsigned int Flags;

        std::unique_ptr<MOPY> MaterialsChunk;
        std::unique_ptr<MOVI> IndicesChunk;
        std::unique_ptr<MOVT> VerticesChunk;
        std::unique_ptr<MLIQ> LiquidChunk;

        // bounding box.  XXX - do we actually need this?
        float Min[3];
        float Max[3];

        WmoGroupFile(const std::string &path);
};
}
}