#include <vector>

#include "utility/Include/BinaryStream.hpp"
#include "Input/Adt/AdtChunk.hpp"
#include "Input/Wmo/WmoParserInfo.hpp"

namespace parser
{
namespace input
{
class MODF : AdtChunk
{
    private:
        static const int WmoSize = 0x40;

    public:
        std::vector<WmoParserInfo> Wmos;

        MODF(long position, utility::BinaryStream *reader);
};
}
}