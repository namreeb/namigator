#pragma once

#include <map>
#include <string>

#include "Input/Wmo/Root File/WmoRootChunk.hpp"

namespace parser_input
{
    class MODN : WmoRootChunk
    {
        public:
            std::map<unsigned int, std::string> Names;

            MODN(unsigned int doodadNamesCount, long position, utility::BinaryStream *reader);
    };
}