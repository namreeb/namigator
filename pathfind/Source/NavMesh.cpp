#include "NavMesh.hpp"
#include "Common.hpp"
#include "utility/Include/MathHelper.hpp"

#include "DetourNavMesh.h"

#include <string>
#include <sstream>
#include <fstream>
#include <cassert>

namespace pathfind
{
NavMesh::NavMesh(const std::string &dataPath, const std::string &continentName) : m_dataPath(dataPath), m_continentName(continentName)
{
    dtNavMeshParams params;

    params.orig[0] = -32.f * RecastSettings::TileSize;
    params.orig[1] = 0.f;
    params.orig[2] = -32.f * RecastSettings::TileSize;
    params.tileHeight = params.tileWidth = RecastSettings::TileVoxelSize;
    params.maxTiles = 64 * 64;
    params.maxPolys = 1 << DT_POLY_BITS;

    auto result = m_navMesh.init(&params);
    assert(result == DT_SUCCESS);
}

bool NavMesh::LoadFile(const std::string &filename, unsigned char **data, int *size)
{
    std::ifstream in(filename, std::ifstream::binary);

    if (in.fail())
        return false;

    in.seekg(0, in.end);
    auto const s = in.tellg();
    *size = static_cast<int>(s);
    in.seekg(0, in.beg);

    // the dtNavMesh destructor will handle deallocation of this data
    *data = new unsigned char[*size];
    in.read(reinterpret_cast<char *>(*data), *size);

    return true;
}

bool NavMesh::LoadTile(int x, int y)
{
    std::stringstream str;
    str << m_dataPath << "\\" << m_continentName << "_" << x << "_" << y << ".map";

    unsigned char *buff;
    int size;

    if (!LoadFile(str.str(), &buff, &size))
        return false;

    return m_navMesh.addTile(buff, size, DT_TILE_FREE_DATA, 0, nullptr) == DT_SUCCESS;
}

bool NavMesh::LoadGlobalWMO()
{
    std::stringstream str;
    str << m_dataPath << "\\" << m_continentName << ".map";

    unsigned char *buff;
    int size;

    if (!LoadFile(str.str(), &buff, &size))
        return false;

    return m_navMesh.addTile(buff, size, DT_TILE_FREE_DATA, 0, nullptr) == DT_SUCCESS;
}

void NavMesh::GetTileGeometry(int x, int y, std::vector<utility::Vertex> &vertices, std::vector<int> &indices)
{
    auto const tile = m_navMesh.getTileAt(x, y, 0);

    unsigned int triCount = 0;

    for (int i = 0; i < tile->header->detailMeshCount; ++i)
        triCount += tile->detailMeshes[i].triCount;

    vertices.reserve(3 * triCount);
    indices.reserve(triCount);

    for (int i = 0; i < tile->header->polyCount; ++i)
    {
        auto const p = &tile->polys[i];

        if (p->areaAndtype == DT_POLYTYPE_OFFMESH_CONNECTION)
            continue;

        auto const pd = &tile->detailMeshes[i];

        for (int j = 0; j < pd->triCount; ++j)
        {
            auto const t = &tile->detailTris[(pd->triBase + j) * 4];

            for (int k = 0; k < 3; ++k)
            {
                auto const vert = t[k] < p->vertCount ? &tile->verts[p->verts[t[k]] * 3] : &tile->detailVerts[(pd->vertBase + t[k] - p->vertCount) * 3];

                vertices.push_back({ -vert[2], -vert[0], vert[1] });
            }

            indices.push_back(vertices.size() - 3);
            indices.push_back(vertices.size() - 2);
            indices.push_back(vertices.size() - 1);
        }
    }
}
}