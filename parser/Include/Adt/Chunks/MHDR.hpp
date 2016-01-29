#include "Adt/AdtChunk.hpp"

#include "utility/Include/BinaryStream.hpp"

#include <memory>
#include <cstdint>

namespace parser
{
namespace input
{
struct MHDRInfo
{
    public:
        std::uint32_t Flags;
        std::uint32_t McinOffset;
        std::uint32_t MtexOffset;
        std::uint32_t MmdxOffset;
        std::uint32_t MmidOffset;
        std::uint32_t MwmoOffset;
        std::uint32_t MwidOffset;
        std::uint32_t MddfOffset;
        std::uint32_t ModfOffset;
        std::uint32_t MfboOffset;
        std::uint32_t Mh2oOffset;
    private:
        std::uint32_t Unknown[5];
};

class MHDR : AdtChunk
{
    public:
        MHDRInfo Offsets;

        MHDR(size_t position, utility::BinaryStream *reader);
};
}
}