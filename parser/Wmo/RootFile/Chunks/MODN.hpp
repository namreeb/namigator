#pragma once

#include "Wmo/RootFile/WmoRootChunk.hpp"
#include "utility/BinaryStream.hpp"

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

    MODN(unsigned int doodadNamesCount, size_t position,
         utility::BinaryStream* reader);
};
} // namespace input
} // namespace parser