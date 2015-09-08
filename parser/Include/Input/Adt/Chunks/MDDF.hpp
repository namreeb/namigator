#pragma once

#include <vector>
#include <memory>

#include "BinaryStream.hpp"

using namespace utility;

#include "Input/Adt/AdtChunk.hpp"
#include "Input/M2/DoodadFile.hpp"

namespace parser_input
{
    class MDDF : AdtChunk
    {
        public:
            std::vector<DoodadInfo> Doodads;

            MDDF(long position, utility::BinaryStream *reader);
    };
}