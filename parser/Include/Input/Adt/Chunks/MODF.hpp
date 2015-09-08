#include <vector>

#include "BinaryStream.hpp"
#include "Input/Adt/AdtChunk.hpp"
#include "Input/Wmo/WmoParserInfo.hpp"

namespace parser_input
{
    class MODF : AdtChunk
    {
        private:
            static const int WmoSize = 0x40;

        public:
            std::vector<std::unique_ptr<WmoParserInfo>> Wmos;

            MODF(long position, utility::BinaryStream *reader);
    };
}