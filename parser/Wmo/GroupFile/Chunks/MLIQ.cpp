#include "Common.hpp"
#include "Wmo/GroupFile/Chunks/MLIQ.hpp"

namespace parser
{
namespace input
{
MLIQ::MLIQ(size_t position, utility::BinaryStream* groupFileStream,
           unsigned int version)
    : WmoGroupChunk(position, groupFileStream)
{
    groupFileStream->rpos(position + 16);

    Width = groupFileStream->Read<std::uint32_t>();
    Height = groupFileStream->Read<std::uint32_t>();

    groupFileStream->ReadBytes(&Corner, sizeof(Corner));

    // Adjustment for alpha data
    if (version == 14)
    {
        constexpr float tileSize = MeshSettings::AdtSize / 128.f;
        Corner[0] -= tileSize * Height;

        auto const w = Width;
        Width = Height;
        Height = w;
    }

    groupFileStream->rpos(groupFileStream->rpos() + 2);

    Heights = std::make_unique<utility::Array2d<float>>(Height + 1, Width + 1);
    for (unsigned int y = 0; y <= Height; ++y)
        for (unsigned int x = 0; x <= Width; ++x)
        {
            groupFileStream->rpos(groupFileStream->rpos() + 4);
            Heights->Set(y, x, groupFileStream->Read<float>());
        }
}
} // namespace input
} // namespace parser