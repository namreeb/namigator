#pragma once
#include <string>
#include <vector>

#include "BinaryStream.hpp"
#include "Input/Adt/AdtChunk.hpp"

namespace parser_input
{
    class MMDX : public AdtChunk
    {
        public:
            std::vector<std::string> DoodadNames;

            MMDX(long position, utility::BinaryStream *reader);
    };
}