#include "utility/Include/BinaryStream.hpp"
#include "Input/Adt/AdtChunk.hpp"

#include <memory>

namespace parser
{
namespace input
{
struct MHDRInfo
{
    public:
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
    private:
        unsigned int Unknown[5];
};

class MHDR : AdtChunk
{
    public:
        MHDRInfo Offsets;

        MHDR(long position, utility::BinaryStream *reader);
};
}
}