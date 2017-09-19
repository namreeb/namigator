#pragma once

#include "utility/BinaryStream.hpp"
#include "Adt/AdtChunk.hpp"

#include <string>
#include <vector>

namespace parser
{
namespace input
{
class MMDX : public AdtChunk
{
    public:
        std::vector<std::string> DoodadNames;

        MMDX(size_t position, utility::BinaryStream *reader);
};
}
}