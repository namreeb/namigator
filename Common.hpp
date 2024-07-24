#pragma once

enum PolyFlags : unsigned char
{
    Ground = 1 << 0,
    Steep = 1 << 1,
    Liquid = 1 << 2,
    Wmo = 1 << 3,
    Doodad = 1 << 4,
};

#include <cstdint>

// WARNING!!!  If these values are changed, existing data must be regenerated.
// It is assumed that the client and generator values match EXACTLY!

struct MeshSettings
{
    static constexpr int MaxMPQPathLength = 128;
    static constexpr int TilesPerChunk =
        1; // number of rows and columns of tiles per ADT MCNK chunk
    static constexpr int TileVoxelSize =
        112; // number of voxel rows and columns per tile

    static constexpr float CellHeight = 0.25f;
    static constexpr float WalkableHeight =
        1.6f; // agent height in world units (yards)
    static constexpr float WalkableRadius =
        0.3f; // narrowest allowable hallway in world units (yards)
    static constexpr float WalkableSlope =
        50.f; // maximum walkable slope, in degrees
    static constexpr float WalkableClimb =
        1.f; // maximum 'step' height for which slope is ignored (yards)
    static constexpr float DetailSampleDistance =
        3.f; // heightfield detail mesh sample distance (yards)
    static constexpr float DetailSampleMaxError =
        0.25f; // maximum distance detail mesh surface should deviate from
               // heightfield (yards)

    // NOTE: If Recast warns "Walk towards polygon center failed to reach
    // center", try lowering this value
    static constexpr float MaxSimplificationError = 0.5f;

    static constexpr int MinRegionSize = 1600;
    static constexpr int MergeRegionSize = 400;
    static constexpr int VerticesPerPolygon = 6;

    static constexpr std::uint32_t FileSignature = 'NNAV';
    static constexpr std::uint32_t FileVersion = '0006';
    static constexpr std::uint32_t FileADT = 'ADT\0';
    static constexpr std::uint32_t FileWMO = 'WMO\0';
    static constexpr std::uint32_t FileMap = 'MAP1';
    static constexpr std::uint32_t WMOcoordinate = 0xFFFFFFFF;

    // Nothing below here should ever have to change

    static constexpr int Adts = 64;
    static constexpr int ChunksPerAdt = 16;
    static constexpr int TilesPerADT = TilesPerChunk * ChunksPerAdt;
    static constexpr int TileCount = Adts * TilesPerADT;
    static constexpr int ChunkCount = Adts * ChunksPerAdt;
    static constexpr int QuadValuesPerTile =
        128 / (TilesPerChunk * TilesPerChunk) + 16 / (TilesPerChunk) + 1;

    static constexpr float AdtSize = 533.f + (1.f / 3.f);
    static constexpr float AdtChunkSize = AdtSize / ChunksPerAdt;
    static constexpr float MaxCoordinate = AdtSize * (Adts / 2);

    static constexpr float TileSize = AdtChunkSize / TilesPerChunk;
    static constexpr float CellSize = TileSize / TileVoxelSize;

    static constexpr int VoxelWalkableRadius =
        static_cast<int>(WalkableRadius / CellSize);
    static constexpr int VoxelWalkableHeight =
        static_cast<int>(WalkableHeight / CellHeight);
    static constexpr int VoxelWalkableClimb =
        static_cast<int>(WalkableClimb / CellHeight);

    static_assert(TilesPerChunk == 1 || TilesPerChunk == 2 ||
                      TilesPerChunk == 4 || TilesPerChunk == 8,
                  "Tiles must evenly divide the chunk height structure");
    static_assert(TileSize <= AdtChunkSize,
                  "Tiles cannot be larger than an ADT chunk");
    static_assert(VoxelWalkableRadius > 0,
                  "VoxelWalkableRadius must be a positive integer");
    static_assert(VoxelWalkableHeight >= 0,
                  "VoxelWalkableHeight must be a non-negativeinteger");
    static_assert(VoxelWalkableClimb >= 0,
                  "VoxelWalkableClimb must be non-negative integer");
    static_assert(CellSize > 0.f, "CellSize must be positive");
};

enum class Result {
    SUCCESS = 0,
    UNRECOGNIZED_EXTENSION = 1,
    NO_MOGP_CHUNK = 2,
    NO_MOPY_CHUNK = 3,
    NO_MOVI_CHUNK = 4,
    NO_MOVT_CHUNK = 5,
    NO_MCNK_CHUNK = 6,

    WDT_OPEN_FAILED = 7,
    MPHD_NOT_FOUND = 8,
    MAIN_NOT_FOUND = 9,
    MDNM_NOT_FOUND = 10,
    MONM_NOT_FOUND = 11,
    MWMO_NOT_FOUND = 12,
    MODF_NOT_FOUND = 13,

    MVER_NOT_FOUND = 14,
    MOMO_NOT_FOUND = 15,
    MOHD_NOT_FOUND = 16,
    MODS_NOT_FOUND = 17,
    MODN_NOT_FOUND = 18,
    MODD_NOT_FOUND = 19,

    MHDR_NOT_FOUND = 20,

    UNRECOGNIZED_DBC_FILE = 21,
    FAILED_TO_OPEN_DBC = 22,

    FAILED_TO_OPEN_ADT = 23,
    ADT_VERSION_IS_INCORRECT = 24,
    MVER_DOES_NOT_BEGIN_ADT_FILE = 25,

    EMPTY_WMO_DOODAD_INSTANTIATED = 26,

    FAILED_TO_OPEN_FILE_FOR_BINARY_STREAM = 27,

    BAD_FORMAT_OF_GAMEOBJECT_FILE = 28,
    FAILED_TO_OPEN_GAMEOBJECT_FILE = 29,
    UNRECOGNIZED_MODEL_EXTENSION = 30,
    GAME_OBJECT_REFERENCES_NON_EXISTENT_MODEL_ID = 31,

    ADT_SERIALIZATION_FAILED_TO_OPEN_OUTPUT_FILE = 32,
    WMO_SERIALIZATION_FAILED_TO_OPEN_OUTPUT_FILE = 33,

    REQUESTED_BVH_NOT_FOUND = 34,
    BVH_INDEX_FILE_NOT_FOUND = 35,

    INCORRECT_FILE_SIGNATURE = 36,
    INCORRECT_FILE_VERSION = 37,
    NOT_WMO_NAV_FILE = 38,
    NOT_WMO_TILE_COORDINATES = 39,
    NOT_ADT_NAV_FILE = 40,
    INVALID_MAP_FILE = 41,
    INCORRECT_WMO_COORDINATES = 42,
    DTNAVMESHQUERY_INIT_FAILED = 43,
    UNKNOWN_WMO_INSTANCE_REQUESTED = 44,
    UNKNOWN_DOODAD_INSTANCE_REQUESTED = 45,
    COULD_NOT_DESERIALIZE_DOODAD = 46,
    COULD_NOT_DESERIALIZE_WMO = 47,
    INCORRECT_ADT_COORDINATES = 48,
    TILE_NOT_FOUND_FOR_REQUESTED = 49,
    COULD_NOT_FIND_WMO = 50,

    DOODAD_PATH_NOT_FOUND = 51,
    UNSUPPORTED_DOOAD_FORMAT = 52,
    MAP_ID_NOT_FOUND = 53,
    WMO_NOT_FOUND = 54,

    BINARYSTREAM_COMPRESS_FAILED = 55,
    MZ_INFLATEINIT_FAILED = 56,
    MZ_INFLATE_FAILED = 57,

    INVALID_ROW_COLUMN_FROM_DBC = 58,

    GAMEOBJECT_WITH_SPECIFIED_GUID_ALREADY_EXISTS = 59,

    TEMPORARY_WMO_OBSTACLES_ARE_NOT_SUPPORTED = 60,
    NO_DOODAD_SET_SPECIFIED_FOR_WMO_GAME_OBJECT = 61,

    BAD_MATRIX_ROW = 62,
    INVALID_MATRIX_MULTIPLICATION = 63,
    ONLY_4X4_MATRIX_IS_SUPPORTED = 64,
    MATRIX_NOT_INVERTIBLE = 65,

    UNEXPECTED_VERSION_MAGIC_IN_ALPHA_MODEL = 66,
    UNEXPECTED_VERSION_SIZE_IN_ALPHA_MODEL = 67,
    UNSUPPORTED_ALPHA_MODEL_VERSION = 68,
    UNEXPECTED_VERTEX_MAGIC_IN_ALPHA_MODEL = 69,
    UNEXPECTED_TRIANGLE_MAGIC_IN_ALPHA_MODEL = 70,
    INVALID_DOODAD_FILE = 71,

    COULD_NOT_OPEN_MPQ = 72,
    GETROOTAREAID_INVALID_ID = 73,
    NO_DATA_FILES_FOUND = 74,
    MPQ_MANAGER_NOT_INIATIALIZED = 75,
    ERROR_IN_SFILEOPENFILEX = 76,
    ERROR_IN_SFILEREADFILE = 77,
    MULTIPLE_CANDIDATES_IN_ALPHA_MPQ = 78,
    TOO_MANY_FILES_IN_ALPHA_MPQ = 79,
    NO_MPQ_CANDIDATE = 80,

    RECAST_FAILURE = 81,

    // C API
    BUFFER_TOO_SMALL = 82,
    UNKNOWN_PATH = 83,
    UNKNOWN_HEIGHT = 84,
    UNKNOWN_ZONE_AND_AREA = 85,
    FAILED_TO_LOAD_ADT = 86,
    MAP_DOES_NOT_HAVE_ADT = 87,
    UNABLE_TO_FIND_RANDOM_POINT_IN_CIRCLE = 88,

    FAILED_TO_FIND_POINT_BETWEEN_VECTORS = 89,

    UNKNOWN_EXCEPTION = 0xFF,
};
