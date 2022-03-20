#include "Adt/AdtChunk.hpp"
#include "Adt/Chunks/Subchunks/MCLQ.hpp"
#include "Adt/Chunks/Subchunks/MCVT.hpp"
#include "utility/BinaryStream.hpp"
#include "utility/Exception.hpp"
#include "utility/Vector.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace parser
{
namespace input
{
constexpr unsigned int HoleFlags[4][4] = {{0x1, 0x10, 0x100, 0x1000},
                                          {0x2, 0x20, 0x200, 0x2000},
                                          {0x4, 0x40, 0x400, 0x4000},
                                          {0x8, 0x80, 0x800, 0x8000}};

class MCNK : public AdtChunk
{
private:
    static constexpr int VertexCount = 9 * 9 + 8 * 8;

public:
    bool HasHoles;
    bool HasWater;

    float MinZ;
    float MaxZ;

    std::uint32_t AreaId;

    bool HoleMap[8][8];
    std::vector<math::Vector3> Positions;
    float Heights[VertexCount];

    std::unique_ptr<MCLQ> LiquidChunk;

    MCNK(size_t position, utility::BinaryStream* reader, bool alpha, int adtX,
         int adtY);
};
} // namespace input
} // namespace parser