#pragma once
#include <string>
#include <vector>

#include "utility/Include/BinaryStream.hpp"
#include "Adt/AdtChunk.hpp"

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