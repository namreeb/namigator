#pragma once

#include "Input/Wmo/Root File/WmoRootChunk.hpp"

#include <map>
#include <string>

namespace parser
{
namespace input
{
class MODN : WmoRootChunk
{
    public:
        std::map<unsigned int, std::string> Names;

        MODN(unsigned int doodadNamesCount, long position, utility::BinaryStream *reader);
};
}
}