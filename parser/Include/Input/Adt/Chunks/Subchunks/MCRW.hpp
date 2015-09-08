#include <vector>

#include "Input/Adt/AdtChunk.hpp"
#include "BinaryStream.hpp"

namespace parser_input
{
    class MCRW : AdtChunk
    {
        public:
            std::vector<unsigned int> WmoIndices;

            MCRW(long position, utility::BinaryStream *reader);
    };
}