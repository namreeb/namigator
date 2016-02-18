#include "Tile.hpp"
#include "Map.hpp"

#include "Detour/Include/DetourNavMesh.h"

#include <fstream>
#include <vector>
#include <utility>
#include <cassert>
#include <cstdint>

namespace pathfind
{
Tile::Tile(Map *map, std::ifstream &in) : m_map(map)
{
    std::uint32_t wmoInstanceCount;
    in.read(reinterpret_cast<char *>(&wmoInstanceCount), sizeof(wmoInstanceCount));

    m_wmoInstances.reserve(wmoInstanceCount);
    for (std::uint32_t i = 0; i < wmoInstanceCount; ++i)
    {
        std::uint32_t wmoId;
        in.read(reinterpret_cast<char *>(&wmoId), sizeof(wmoId));
        m_wmoInstances.push_back(static_cast<unsigned int>(wmoId));
        m_wmos.push_back(m_map->LoadWmoModel(static_cast<unsigned int>(wmoId)));
    }

    std::uint32_t doodadInstanceCount;
    in.read(reinterpret_cast<char *>(&doodadInstanceCount), sizeof(doodadInstanceCount));

    m_doodadInstances.reserve(doodadInstanceCount);
    for (std::uint32_t i = 0; i < doodadInstanceCount; ++i)
    {
        std::uint32_t doodadId;
        in.read(reinterpret_cast<char *>(&doodadId), sizeof(doodadId));
        m_doodadInstances.push_back(static_cast<unsigned int>(doodadId));
        m_doodads.push_back(m_map->LoadDoodadModel(static_cast<unsigned int>(doodadId)));
    }

    std::int32_t navMeshSize;
    in.read(reinterpret_cast<char *>(&navMeshSize), sizeof(navMeshSize));

    m_tileData.resize(static_cast<size_t>(navMeshSize));
    in.read(reinterpret_cast<char *>(&m_tileData[0]), navMeshSize);

    assert(m_map->m_navMesh.addTile(&m_tileData[0], static_cast<int>(navMeshSize), 0, 0, &m_ref) == DT_SUCCESS);
}

Tile::~Tile()
{
    assert(m_map->m_navMesh.removeTile(m_ref, nullptr, nullptr) == DT_SUCCESS);
}
}