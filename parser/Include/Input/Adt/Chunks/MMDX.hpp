#pragma once
#include <string>
#include <vector>

#include "utility/Include/BinaryStream.hpp"
#include "Input/Adt/AdtChunk.hpp"

namespace parser
{
namespace input
{
class MMDX : public AdtChunk
{
    public:
        std::vector<std::string> DoodadNames;

        MMDX(long position, utility::BinaryStream *reader);
};
}
}