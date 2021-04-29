#pragma once

#include "Common.hpp"
#include "Model.hpp"
#include "recastnavigation/Detour/Include/DetourNavMesh.h"
#include "recastnavigation/Recast/Include/Recast.h"
#include "utility/BinaryStream.hpp"
#include "utility/BoundingBox.hpp"
#include "utility/Ray.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

namespace pathfind
{
class Map;

class Tile
{
private:
    Map* const m_map;
    const fs::path m_navPath;

    std::vector<std::uint8_t> m_tileData;

    // store this for possible delayed load of the data
    size_t m_heightFieldSpanStart;
    rcHeightfield m_heightField;

    void LoadHeightField(utility::BinaryStream& in);
    void LoadHeightField();

public:
    // the height field should only be loaded for tiles that will have temporary
    // obstacles inserted frequently
    Tile(Map* map, utility::BinaryStream& in, const fs::path& navPath,
         bool load_heightfield = false);
    ~Tile();

    void AddTemporaryDoodad(std::uint64_t guid,
                            std::shared_ptr<DoodadInstance> doodad);

    dtTileRef m_ref;

    math::BoundingBox m_bounds;

    const int m_x;
    const int m_y;

    std::uint32_t m_zoneId;
    std::uint32_t m_areaId;

    std::uint8_t m_quadHoles[8 / MeshSettings::TilesPerChunk]
                            [8 / MeshSettings::TilesPerChunk];
    std::vector<float> m_quadHeights;

    // static instance ids, loaded per map
    std::vector<std::uint32_t> m_staticWmos;
    std::vector<std::uint32_t> m_staticDoodads;

    // park the shared pointers here just to increment their reference counts
    std::vector<std::shared_ptr<WmoModel>> m_staticWmoModels;
    std::vector<std::shared_ptr<DoodadModel>> m_staticDoodadModels;

    // indxed by GUID
    std::unordered_map<std::uint64_t, std::shared_ptr<WmoInstance>>
        m_temporaryWmos;
    std::unordered_map<std::uint64_t, std::shared_ptr<DoodadInstance>>
        m_temporaryDoodads;
};
} // namespace pathfind