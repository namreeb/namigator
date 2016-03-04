#pragma once

enum AreaFlags : unsigned char
{
    Walkable    = 1 << 0,
    ADT         = 1 << 1,
    Liquid      = 1 << 2,
    WMO         = 1 << 3,
    Doodad      = 1 << 4,
};

using PolyFlags = AreaFlags;

class RecastSettings
{
    public:
        static constexpr int TileVoxelSize = 1000;

        static constexpr float CellHeight = 0.4f;
        static constexpr float WalkableHeight = 1.6f;   // agent height in world units (yards)
        static constexpr float WalkableRadius = 0.3f;   // narrowest allowable hallway in world units (yards)
        static constexpr float WalkableSlope = 50.f;    // maximum walkable slope, in degrees
        static constexpr float WalkableClimb = 1.f;     // maximum 'step' height for which slope is ignored (yards)

        static constexpr float MaxSimplificationError = 1.3f;

        static constexpr int MinRegionSize = 45;
        static constexpr int MergeRegionSize = 75;

        static constexpr float TileSize = 533.f + (1.f / 3.f);
        static constexpr int VoxelWalkableRadius = static_cast<int>((WalkableRadius + 0.5f) / (TileSize / TileVoxelSize));
        static constexpr int VoxelWalkableHeight = static_cast<int>((WalkableHeight + 0.5f) / RecastSettings::CellHeight);
        static constexpr int VoxelWalkableClimb = static_cast<int>((WalkableClimb + 0.5f) / RecastSettings::CellHeight);
        static constexpr float CellSize = TileSize / TileVoxelSize;

        static_assert(VoxelWalkableRadius > 0, "VoxelWalkableRadius must be a positive integer");
        static_assert(VoxelWalkableHeight > 0, "VoxelWalkableHeight must be a positive integer");
        static_assert(VoxelWalkableClimb >= 0, "VoxelWalkableClimb must be non-negative integer");
        static_assert(CellSize > 0.f, "CellSize must be positive");
};
