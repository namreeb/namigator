#pragma once

#include <cstdint>

#pragma pack(push, 1)
struct AdtChunkLocation
{
    std::uint8_t AdtX;
    std::uint8_t AdtY;
    std::uint8_t ChunkX;
    std::uint8_t ChunkY;

    bool operator<(const AdtChunkLocation& other) const
    {
        return *reinterpret_cast<const std::uint32_t*>(this) <
               *reinterpret_cast<const std::uint32_t*>(&other);
    }
};
#pragma pack(pop)

static_assert(sizeof(AdtChunkLocation) == 4,
              "Unexpected packing of AdtChunkLocation");