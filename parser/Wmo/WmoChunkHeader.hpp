#pragma once

#include <cstdint>

namespace parser
{
namespace input
{
struct WmoChunkHeader
{
    std::uint32_t Type;
    std::uint32_t Size;
};
}
}