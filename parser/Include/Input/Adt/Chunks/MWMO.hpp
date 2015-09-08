#include <vector>

#include "Input/Adt/AdtChunk.hpp"

namespace parser_input
{
    class MWMO : AdtChunk
    {
        public:
            std::vector<std::string> WmoNames;

            MWMO(long position, utility::BinaryStream *reader);
    };
}