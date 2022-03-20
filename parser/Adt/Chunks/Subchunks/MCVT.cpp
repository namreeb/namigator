#include "Adt/Chunks/Subchunks/MCVT.hpp"

#include "utility/Exception.hpp"

#include <cassert>

namespace parser
{
namespace input
{
MCVT::MCVT(size_t position, utility::BinaryStream* reader)
    : AdtChunk(position, reader)
{
    const bool alpha = Type != AdtChunkType::MCVT;

    // true for alpha data embedded in WDT
    if (alpha)
        reader->rpos(position);

    reader->ReadBytes(&Heights, sizeof(Heights));

    // alpha vertices are not interleaved.  rather than processing it natively,
    // its simpler to interleave them ourselves.  this simplifies further
    // processing by us later which assumes this layout.
    if (alpha)
    {
        auto constexpr floats = sizeof(Heights) / sizeof(Heights[0]);

        std::vector<float> interleaved;
        interleaved.reserve(floats);
        int nextOuter = 0, nextInner = 0;
        for (int row = 0; row < 9; ++row)
        {
            // outer
            for (int i = 0; i < 9; ++i)
                interleaved.push_back(Heights[nextOuter++]);
            if (row != 8)
                for (int i = 0; i < 8; ++i)
                    interleaved.push_back(Heights[81 + nextInner++]);
        }
        memcpy(Heights, &interleaved[0], sizeof(Heights));
    }
}
} // namespace input
} // namespace parser