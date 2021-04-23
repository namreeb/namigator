#pragma once

enum PolyFlags : unsigned char
{
    ADT         = 1 << 0,
    Liquid      = 1 << 1,
    WMO         = 1 << 2,
    Doodad      = 1 << 3,
    Steep       = 1 << 4,
};

// WARNING!!!  If these values are changed, existing data must be regenerated.  It is assumed that the client and generator values match EXACTLY!

struct MeshSettings
{
    static constexpr int MaxMPQPathLength = 128;
    static constexpr int TilesPerChunk = 1;                 // number of rows and columns of tiles per ADT MCNK chunk
    static constexpr int TileVoxelSize = 112;               // number of voxel rows and columns per tile

    static constexpr float CellHeight = 0.25f;
    static constexpr float WalkableHeight = 1.6f;           // agent height in world units (yards)
    static constexpr float WalkableRadius = 0.3f;           // narrowest allowable hallway in world units (yards)
    static constexpr float WalkableSlope = 50.f;            // maximum walkable slope, in degrees
    static constexpr float WalkableClimb = 1.f;             // maximum 'step' height for which slope is ignored (yards)
    static constexpr float DetailSampleDistance = 3.f;      // heightfield detail mesh sample distance (yards)
    static constexpr float DetailSampleMaxError = 0.25f;    // maximum distance detail mesh surface should deviate from heightfield (yards)

    // NOTE: If Recast warns "Walk towards polygon center failed to reach center", try lowering this value
    static constexpr float MaxSimplificationError = 0.5f;

    static constexpr int MinRegionSize = 1600;
    static constexpr int MergeRegionSize = 400;
    static constexpr int VerticesPerPolygon = 6;

    static constexpr std::uint32_t FileSignature = 'NNAV';
    static constexpr std::uint32_t FileVersion = '0005';
    static constexpr std::uint32_t FileADT = 'ADT\0';
    static constexpr std::uint32_t FileWMO = 'WMO\0';
    static constexpr std::uint32_t WMOcoordinate = 0xFFFFFFFF;

    // Nothing below here should ever have to change

    static constexpr int Adts = 64;
    static constexpr int ChunksPerAdt = 16;
    static constexpr int TilesPerADT = TilesPerChunk*ChunksPerAdt;
    static constexpr int TileCount = Adts * TilesPerADT;
    static constexpr int ChunkCount = Adts * ChunksPerAdt;
    static constexpr int QuadValuesPerTile = 128 / (TilesPerChunk*TilesPerChunk) + 16 / (TilesPerChunk) + 1;

    static constexpr float AdtSize = 533.f + (1.f / 3.f);
    static constexpr float AdtChunkSize = AdtSize / ChunksPerAdt;

    static constexpr float TileSize = AdtChunkSize / TilesPerChunk;
    static constexpr float CellSize = TileSize / TileVoxelSize;

    static constexpr int VoxelWalkableRadius = static_cast<int>(WalkableRadius / CellSize);
    static constexpr int VoxelWalkableHeight = static_cast<int>(WalkableHeight / CellHeight);
    static constexpr int VoxelWalkableClimb = static_cast<int>(WalkableClimb / CellHeight);

    static_assert(
        TilesPerChunk == 1 ||
        TilesPerChunk == 2 ||
        TilesPerChunk == 4 ||
        TilesPerChunk == 8, "Tiles must evenly divide the chunk height structure");
    static_assert(TileSize <= AdtChunkSize, "Tiles cannot be larger than an ADT chunk");
    static_assert(VoxelWalkableRadius > 0, "VoxelWalkableRadius must be a positive integer");
    static_assert(VoxelWalkableHeight > 0, "VoxelWalkableHeight must be a positive integer");
    static_assert(VoxelWalkableClimb >= 0, "VoxelWalkableClimb must be non-negative integer");
    static_assert(CellSize > 0.f, "CellSize must be positive");
};
