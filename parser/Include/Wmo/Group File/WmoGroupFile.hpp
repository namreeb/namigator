#pragma once

#include "Wmo/Group File/Chunks/MOPY.hpp"
#include "Wmo/Group File/Chunks/MOVI.hpp"
#include "Wmo/Group File/Chunks/MOVT.hpp"
#include "Wmo/Group File/Chunks/MLIQ.hpp"

#include <string>
#include <memory>

namespace parser
{
namespace input
{
class WmoGroupFile
{
    public:
        unsigned int Flags;

        std::unique_ptr<MOPY> MaterialsChunk;
        std::unique_ptr<MOVI> IndicesChunk;
        std::unique_ptr<MOVT> VerticesChunk;
        std::unique_ptr<MLIQ> LiquidChunk;

        WmoGroupFile(const std::string &path);
};
}
}