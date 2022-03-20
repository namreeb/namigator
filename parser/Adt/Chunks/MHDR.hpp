#include "Adt/AdtChunk.hpp"
#include "utility/BinaryStream.hpp"

#include <cstdint>
#include <memory>

namespace parser
{
namespace input
{
class MHDR : AdtChunk
{
public:
    std::uint32_t Mh2oOffset;
    std::uint32_t DoodadNamesOffset;
    std::uint32_t WmoNamesOffset;
    std::uint32_t DoodadPlacementOffset;
    std::uint32_t WmoPlacementOffset;

    MHDR(size_t position, utility::BinaryStream* reader, bool alpha);
};
} // namespace input
} // namespace parser