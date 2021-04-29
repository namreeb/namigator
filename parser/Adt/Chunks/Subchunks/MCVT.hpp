#include "Adt/AdtChunk.hpp"
#include "utility/BinaryStream.hpp"

namespace parser
{
namespace input
{
class MCVT : AdtChunk
{
public:
    float Heights[8 * 8 + 9 * 9];

    MCVT(size_t position, utility::BinaryStream* reader);
};
} // namespace input
} // namespace parser