#include "Input/Adt/AdtChunk.hpp"
#include "BinaryStream.hpp"

using namespace utility;

namespace parser_input
{
    class MCVT : AdtChunk
    {
        public:
            float Heights[8*8 + 9*9];

            MCVT(long position, utility::BinaryStream *reader);
    };
}