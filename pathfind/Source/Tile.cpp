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

    std::vector<std::uint32_t> wmoInstances(wmoInstanceCount);
    in.read(reinterpret_cast<char *>(&wmoInstances[0]), wmoInstanceCount * sizeof(std::uint32_t));

    for (auto const &wmoId : wmoInstances)
        m_wmos.push_back(m_map->LoadWmoModel(static_cast<unsigned int>(wmoId)));

    std::uint32_t doodadInstanceCount;
    in.read(reinterpret_cast<char *>(&doodadInstanceCount), sizeof(doodadInstanceCount));

    std::vector<std::uint32_t> doodadInstances(doodadInstanceCount);
    in.read(reinterpret_cast<char *>(&doodadInstances[0]), doodadInstanceCount * sizeof(std::uint32_t));

    for (auto const &doodadId : doodadInstances)
        m_doodads.push_back(m_map->LoadDoodadModel(static_cast<unsigned int>(doodadId)));

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