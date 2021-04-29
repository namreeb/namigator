#include "Tile.hpp"

#include "Common.hpp"
#include "Map.hpp"
#include "MapBuilder/MeshBuilder.hpp"
#include "recastnavigation/Detour/Include/DetourNavMesh.h"
#include "recastnavigation/Recast/Include/Recast.h"
#include "recastnavigation/Recast/Include/RecastAlloc.h"
#include "utility/BinaryStream.hpp"
#include "utility/Exception.hpp"
#include "utility/MathHelper.hpp"

#include <cassert>
#include <cstdint>
#include <vector>

namespace pathfind
{
Tile::Tile(Map* map, utility::BinaryStream& in, const fs::path& navPath,
           bool load_heightfield)
    : m_map(map), m_navPath(navPath), m_ref(0), m_x(in.Read<std::uint32_t>()),
      m_y(in.Read<std::uint32_t>()), m_areaId(0)
{
    std::uint32_t wmoCount;
    in >> wmoCount;

    if (wmoCount > 0)
    {
        m_staticWmos.resize(wmoCount);
        in.ReadBytes(&m_staticWmos[0],
                     m_staticWmos.size() * sizeof(std::uint32_t));

        for (auto const wmo : m_staticWmos)
            m_staticWmoModels.push_back(
                std::move(map->LoadModelForWmoInstance(wmo)));
    }

    // for global WMOs, doodads are not referenced or loaded on a per-tile
    // basis, therefore this will always be zero in that case
    std::uint32_t doodadCount;
    in >> doodadCount;

    if (doodadCount > 0)
    {
        m_staticDoodads.resize(doodadCount);
        in.ReadBytes(&m_staticDoodads[0],
                     m_staticDoodads.size() * sizeof(std::uint32_t));

        for (auto const doodad : m_staticDoodads)
            m_staticDoodadModels.push_back(
                std::move(map->LoadModelForDoodadInstance(doodad)));
    }

    std::uint8_t quadHeight;
    in >> quadHeight;

    // only two possible values
    assert(quadHeight == 0 || quadHeight == 1);

    // optional quad height data for ADT based tiles
    if (quadHeight)
    {
        in >> m_zoneId;
        in >> m_areaId;

        in.ReadBytes(&m_quadHoles, sizeof(m_quadHoles));
        m_quadHeights.resize(MeshSettings::QuadValuesPerTile);
        in.ReadBytes(&m_quadHeights[0], sizeof(float) * m_quadHeights.size());
    }

    // read height field
    in >> m_heightField.width >> m_heightField.height >> m_heightField.bmin >>
        m_heightField.bmax >> m_heightField.cs >> m_heightField.ch;

    // for now, width and height must always be equal.  this check is here as a
    // way to make sure we are reading the file correctly so far
    assert(m_heightField.width == m_heightField.height);

    math::Vector3 a, b;
    math::Convert::VertexToWow(m_heightField.bmin, a);
    math::Convert::VertexToWow(m_heightField.bmax, b);

    m_bounds.MinCorner.X = (std::min)(a.X, b.X);
    m_bounds.MaxCorner.X = (std::max)(a.X, b.X);
    m_bounds.MinCorner.Y = (std::min)(a.Y, b.Y);
    m_bounds.MaxCorner.Y = (std::max)(a.Y, b.Y);
    m_bounds.MinCorner.Z = (std::min)(a.Z, b.Z);
    m_bounds.MaxCorner.Z = (std::max)(a.Z, b.Z);

    m_heightFieldSpanStart = in.rpos();

    if (load_heightfield)
        LoadHeightField(in);
    else
    {
        m_heightField.spans = nullptr;

        for (auto i = 0; i < m_heightField.width * m_heightField.height; ++i)
        {
            std::uint32_t columnSize;
            in >> columnSize;

            in.rpos(in.rpos() + 3 * columnSize * sizeof(std::uint32_t));
        }
    }

    // read mesh
    std::uint32_t meshSize;
    in >> meshSize;

    if (meshSize > 0)
    {
        m_tileData.resize(meshSize);
        in.ReadBytes(&m_tileData[0], m_tileData.size());

        auto const result = m_map->m_navMesh.addTile(
            &m_tileData[0], static_cast<int>(m_tileData.size()), 0, 0, &m_ref);
        assert(result == DT_SUCCESS);
    }
}

Tile::~Tile()
{
    if (!!m_ref)
    {
        auto const result =
            m_map->m_navMesh.removeTile(m_ref, nullptr, nullptr);
        assert(result == DT_SUCCESS);
    }

    if (!!m_heightField.spans)
        for (auto i = 0; i < m_heightField.width * m_heightField.height; ++i)
            rcFree(m_heightField.spans[i]);
}

void Tile::LoadHeightField()
{
    utility::BinaryStream in(m_navPath);
    in.rpos(m_heightFieldSpanStart);
    LoadHeightField(in);
}

void Tile::LoadHeightField(utility::BinaryStream& in)
{
    assert(!m_heightField.spans);

    m_heightField.spans = reinterpret_cast<rcSpan**>(
        rcAlloc(m_heightField.width * m_heightField.height * sizeof(rcSpan*),
                RC_ALLOC_PERM));

    for (auto i = 0; i < m_heightField.width * m_heightField.height; ++i)
    {
        std::uint32_t columnSize;
        in >> columnSize;

        if (!columnSize)
        {
            m_heightField.spans[i] = nullptr;
            continue;
        }

        m_heightField.spans[i] = reinterpret_cast<rcSpan*>(
            rcAlloc(columnSize * sizeof(rcSpan), RC_ALLOC_PERM));

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
}
} // namespace pathfind
