#include <memory>
#include "BinaryStream.hpp"
#include "Input/Adt/AdtChunk.hpp"

namespace parser_input
{
    struct MHDRInfo
    {
        unsigned int Flags;
        unsigned int McinOffset;
        unsigned int MtexOffset;
        unsigned int MmdxOffset;
        unsigned int MmidOffset;
        unsigned int MwmoOffset;
        unsigned int MwidOffset;
        unsigned int MddfOffset;
        unsigned int ModfOffset;
        unsigned int MfboOffset;
        unsigned int Mh2oOffset;

        unsigned int Unknown[5];
    };

    class MHDR : AdtChunk
    {
        public:
            std::unique_ptr<MHDRInfo> Offsets;

            MHDR(long position, utility::BinaryStream *reader);
    };
}