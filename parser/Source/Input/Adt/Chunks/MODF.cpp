#include "Input/ADT/Chunks/MODF.hpp"

namespace parser_input
{
    MODF::MODF(long position, utility::BinaryStream *reader) : AdtChunk(position, reader)
    {
        const int count = Size / WmoSize;
        Wmos.reserve(count);

        // 64 (0x40) bytes per WMO instance
        for (long readerPos = position + 8, i = 0; i < count; readerPos += 0x40, ++i)
        {
            reader->SetPosition(readerPos);
            Wmos.push_back(std::unique_ptr<WmoParserInfo>(reader->AllocateAndReadStruct<WmoParserInfo>()));
        }
    }
}