#include "Tile.hpp"
#include "Map.hpp"

#include "utility/Include/Exception.hpp"
#include "utility/Include/BinaryStream.hpp"
#include "utility/Include/MathHelper.hpp"

#include "MapBuilder/Include/MeshBuilder.hpp"

#include "recastnavigation/Recast/Include/Recast.h"
#include "recastnavigation/Recast/Include/RecastAlloc.h"
#include "recastnavigation/Detour/Include/DetourNavMesh.h"

#include "RecastDetourBuild/Include/Common.hpp"

#include <vector>
#include <cassert>
#include <cstdint>

namespace pathfind
{
Tile::Tile(Map *map, utility::BinaryStream &in) : m_map(map), m_ref(0), m_x(-1), m_y(-1)
{
    std::uint32_t tileX, tileY;
    in >> tileX >> tileY;

    m_x = static_cast<int>(tileX);
    m_y = static_cast<int>(tileY);

    std::uint32_t wmoCount;
    in >> wmoCount;

    if (wmoCount > 0)
    {
        m_staticWmos.resize(wmoCount);
        in.ReadBytes(&m_staticWmos[0], m_staticWmos.size() * sizeof(std::uint32_t));

        for (auto const wmo : m_staticWmos)
            m_staticWmoModels.push_back(std::move(map->LoadModelForWmoInstance(wmo)));
    }

    // for global WMOs, doodads are not referenced or loaded on a per-tile basis, therefore this will always be zero in that case
    std::uint32_t doodadCount;
    in >> doodadCount;

    if (doodadCount > 0)
    {
        m_staticDoodads.resize(doodadCount);
        in.ReadBytes(&m_staticDoodads[0], m_staticDoodads.size() * sizeof(std::uint32_t));

        for (auto const doodad : m_staticDoodads)
            m_staticDoodadModels.push_back(std::move(map->LoadModelForDoodadInstance(doodad)));
    }

    std::uint8_t quadHeight;
    in >> quadHeight;

    // only two possible values
    assert(quadHeight == 0 || quadHeight == 1);

    // optional quad height data for ADT based tiles
    if (quadHeight)
    {
        in.ReadBytes(&m_quadHoles, sizeof(m_quadHoles));
        m_quadHeights.resize(MeshSettings::QuadValuesPerTile);
        in.ReadBytes(&m_quadHeights[0], sizeof(float)*m_quadHeights.size());
    }

    // read height field
    in >> m_heightField.width >> m_heightField.height >> m_heightField.bmin >> m_heightField.bmax >> m_heightField.cs >> m_heightField.ch;

    // for now, width and height must always be equal.  this check is here as a way to make sure we are reading the file correctly so far
    assert(m_heightField.width == m_heightField.height);

    utility::Vertex a, b;
    utility::Convert::VertexToWow(m_heightField.bmin, a);
    utility::Convert::VertexToWow(m_heightField.bmax, b);

    m_bounds.MinCorner.X = (std::min)(a.X, b.X);
    m_bounds.MaxCorner.X = (std::max)(a.X, b.X);
    m_bounds.MinCorner.Y = (std::min)(a.Y, b.Y);
    m_bounds.MaxCorner.Y = (std::max)(a.Y, b.Y);
    m_bounds.MinCorner.Z = (std::min)(a.Z, b.Z);
    m_bounds.MaxCorner.Z = (std::max)(a.Z, b.Z);

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
