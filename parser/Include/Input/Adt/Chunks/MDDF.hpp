#pragma once

#include <vector>
#include <memory>

#include "utility/Include/BinaryStream.hpp"

#include "Input/Adt/AdtChunk.hpp"
#include "Input/M2/DoodadFile.hpp"

namespace parser
{
namespace input
{
class MDDF : AdtChunk
{
    public:
        std::vector<DoodadInfo> Doodads;

        MDDF(long position, utility::BinaryStream *reader);
};
}
}