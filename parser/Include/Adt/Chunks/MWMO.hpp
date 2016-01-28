#include "Adt/AdtChunk.hpp"

#include "utility/Include/BinaryStream.hpp"

#include <vector>
#include <string>

namespace parser
{
namespace input
{
class MWMO : AdtChunk
{
    public:
        std::vector<std::string> WmoNames;

        MWMO(size_t position, utility::BinaryStream *reader);
};
}
}