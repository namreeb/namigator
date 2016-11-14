#pragma once

#include "Model.hpp"

#include "utility/Include/BinaryStream.hpp"
#include "Recast/Include/Recast.h"
#include "Detour/Include/DetourNavMesh.h"

#include <vector>
#include <memory>

namespace pathfind
{
class Map;

class Tile
{
    private:
        Map * const m_map;
        dtTileRef m_ref;

        int m_x;
        int m_y;

        rcHeightfield m_heightField;
        std::vector<std::uint8_t> m_tileData;

    public:
        Tile(Map *map, utility::BinaryStream &in);
        ~Tile();

        int X() const { return m_x; }
        int Y() const { return m_y; }

        std::vector<std::uint32_t> m_wmos;
        std::vector<std::uint32_t> m_doodads;
};
}