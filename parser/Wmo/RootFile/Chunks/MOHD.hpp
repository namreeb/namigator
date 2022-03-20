#pragma once

#include <cstdint>

namespace parser
{
namespace input
{
struct MOHD
{
    std::uint32_t TexturesCount;
    std::int32_t WMOGroupFilesCount;
    std::uint32_t PortalsCount;
    std::uint32_t LightsCount;
    std::uint32_t DoodadNamesCount;
    std::uint32_t DoodadDefsCount;
    std::uint32_t DoodadSetsCount;

    std::uint8_t ColR;
    std::uint8_t ColG;
    std::uint8_t ColB;
    std::uint8_t ColX;

    std::uint32_t WMOId;
};
} // namespace input
} // namespace parser