#pragma once

#include "Model.hpp"

#include "utility/Ray.hpp"
#include "utility/BinaryStream.hpp"
#include "utility/BoundingBox.hpp"

#include "recastnavigation/Recast/Include/Recast.h"
#include "recastnavigation/Detour/Include/DetourNavMesh.h"

#include "RecastDetourBuild/Common.hpp"

#include <unordered_map>
#include <vector>
#include <memory>
#include <cstdint>

namespace pathfind
{
class Map;

class Tile
{
    public:
        Tile(Map *map, utility::BinaryStream &in);
        ~Tile();

        void AddTemporaryDoodad(std::uint64_t guid, std::shared_ptr<DoodadInstance> doodad);

        Map * const m_map;
        dtTileRef m_ref;

        math::BoundingBox m_bounds;

        int m_x;
        int m_y;

        std::uint8_t m_quadHoles[8 / MeshSettings::TilesPerChunk][8 / MeshSettings::TilesPerChunk];     // FIXME: make vector so as not to hard-code size
        std::vector<float> m_quadHeights;

        rcHeightfield m_heightField;
        std::vector<std::uint8_t> m_tileData;

        // static instance ids, loaded per map
        std::vector<std::uint32_t> m_staticWmos;
        std::vector<std::uint32_t> m_staticDoodads;

        // park the shared pointers here just to increment their reference counts
        std::vector<std::shared_ptr<WmoModel>> m_staticWmoModels;
        std::vector<std::shared_ptr<DoodadModel>> m_staticDoodadModels;

        // indxed by GUID
        std::unordered_map<std::uint64_t, std::shared_ptr<WmoInstance>> m_temporaryWmos;
        std::unordered_map<std::uint64_t, std::shared_ptr<DoodadInstance>> m_temporaryDoodads;
};
}