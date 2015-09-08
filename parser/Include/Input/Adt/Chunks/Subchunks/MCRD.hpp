#include <vector>

#include "Input/Adt/AdtChunk.hpp"
#include "BinaryStream.hpp"

namespace parser_input
{
    class MCRD : AdtChunk
    {
        public:
            std::vector<unsigned int> DoodadIndices;

            MCRD(long position, utility::BinaryStream *reader);
    };
}