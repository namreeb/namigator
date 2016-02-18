#pragma once

#include "Model.hpp"

#include "Detour/Include/DetourNavMesh.h"

#include <vector>
#include <memory>

namespace pathfind
{
class Map;

class Tile
{
    private:
        std::vector<std::shared_ptr<WmoModel>> m_wmos;
        std::vector<std::shared_ptr<DoodadModel>> m_doodads;

        std::vector<unsigned char> m_tileData;

        Map * const m_map;
        dtTileRef m_ref;

    public:
        Tile(Map *map, std::ifstream &in);
        ~Tile();

        std::vector<unsigned int> m_wmoInstances;
        std::vector<unsigned int> m_doodadInstances;
};
}