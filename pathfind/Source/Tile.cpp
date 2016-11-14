#include "Tile.hpp"
#include "Map.hpp"

#include "utility/Include/Exception.hpp"
#include "utility/Include/BinaryStream.hpp"
#include "Recast/Include/Recast.h"
#include "Recast/Include/RecastAlloc.h"
#include "Detour/Include/DetourNavMesh.h"

#include <vector>
#include <cassert>
#include <cstdint>

namespace pathfind
{
Tile::Tile(Map *map, utility::BinaryStream &in) : m_map(map), m_ref(0)
{
    std::uint32_t tileX, tileY;
    in >> tileX >> tileY;

    m_x = static_cast<int>(tileX);
    m_y = static_cast<int>(tileY);

    std::uint32_t wmoCount;
    in >> wmoCount;

    m_wmos.resize(wmoCount);
    if (wmoCount > 0)
        in.ReadBytes(&m_wmos[0], m_wmos.size() * sizeof(std::uint32_t));

    // this will be zero for global wmos, even though it will contain doodads.  they are not currently sorted by tile.
    std::uint32_t doodadCount;
    in >> doodadCount;

    m_doodads.resize(doodadCount);
    if (doodadCount > 0)
        in.ReadBytes(&m_doodads[0], m_doodads.size() * sizeof(std::uint32_t));

    // read height field
    in >> m_heightField.width >> m_heightField.height >> m_heightField.bmin >> m_heightField.bmax >> m_heightField.cs >> m_heightField.ch;
    
    m_heightField.spans = reinterpret_cast<rcSpan **>(rcAlloc(m_heightField.width*m_heightField.height * sizeof(rcSpan*), RC_ALLOC_PERM));

    for (auto i = 0; i < m_heightField.width*m_heightField.height; ++i)
    {
        std::uint32_t columnSize;
        in >> columnSize;

        if (!columnSize)
        {
            m_heightField.spans[i] = nullptr;
            continue;
        }

        m_heightField.spans[i] = reinterpret_cast<rcSpan *>(rcAlloc(columnSize * sizeof(rcSpan), RC_ALLOC_PERM));

        for (auto s = 0u; s < columnSize; ++s)
        {
            std::uint32_t smin, smax, area;
            in >> smin >> smax >> area;

            m_heightField.spans[i][s].smin = smin;
            m_heightField.spans[i][s].smax = smax;
            m_heightField.spans[i][s].area = area;
            m_heightField.spans[i][s].next = nullptr;

            if (s > 0)
                m_heightField.spans[i][s - 1].next = &m_heightField.spans[i][s];
        }
    }

    // read mesh
    std::uint32_t meshSize;
    in >> meshSize;

    if (meshSize > 0)
    {
        m_tileData.resize(meshSize);
        in.ReadBytes(&m_tileData[0], m_tileData.size());

        auto const result = m_map->m_navMesh.addTile(&m_tileData[0], static_cast<int>(m_tileData.size()), 0, 0, &m_ref);
        assert(result == DT_SUCCESS);
    }
}

Tile::~Tile()
{
    if (!!m_ref)
    {
        auto const result = m_map->m_navMesh.removeTile(m_ref, nullptr, nullptr);
        assert(result == DT_SUCCESS);
    }

    for (auto i = 0; i < m_heightField.width*m_heightField.height; ++i)
        rcFree(m_heightField.spans[i]);
}
}