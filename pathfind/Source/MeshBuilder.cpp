#include "MeshBuilder.hpp"
#include "Recast.h"

#include <cassert>

#define ZERO(x) memset(&x, 0, sizeof(x));

namespace pathfind
{
namespace build
{
MeshBuilder::MeshBuilder(DataManager *dataManager) : m_dataManager(dataManager) {}

// r+d -> wow: (x, y, z) -> (-z, -x, y)
void MeshBuilder::ConvertVerticesToRecast(const std::vector<utility::Vertex> &input, std::vector<float> &output)
{
    output.resize(input.size() * 3);

    for (size_t i = 0; i < input.size(); ++i)
    {
        output[i * 3 + 0] = -input[i].Z;
        output[i * 3 + 1] = -input[i].X;
        output[i * 3 + 2] = input[i].Y;
    }
}

// wow -> r+d: (x, y, z) -> (-y, z, -x)
void MeshBuilder::ConvertVerticesToWow(const std::vector<float> &input, std::vector<utility::Vertex> &output)
{
    throw "Not implemented yet";
}

void MeshBuilder::ConvertToShort(const std::vector<int> &input, std::vector<unsigned short> &output)
{
    output.resize(input.size());

    for (size_t i = 0; i < input.size(); ++i)
        output[i] = static_cast<unsigned short>(input[i]);
}

bool MeshBuilder::GenerateTile(int adtX, int adtY)
{
    auto const adt = m_dataManager->m_continent->LoadAdt(adtX, adtY);

    assert(adt);

    rcConfig config;
    ZERO(config);

    config.cs = TileSize / static_cast<float>(TileVoxelSize);
    config.ch = CellHeight;
    config.walkableSlopeAngle = 90.f;
    config.walkableHeight = static_cast<int>(std::round(WalkableHeight / CellHeight));
    config.walkableClimb = std::numeric_limits<int>::max();
    config.walkableRadius = static_cast<int>(std::round(WalkableRadius / config.cs));
    config.maxEdgeLen = config.walkableRadius * 8;
    config.maxSimplificationError = MaxSimplificationError;
    config.minRegionArea = 20;
    config.mergeRegionArea = 40;
    config.maxVertsPerPoly = 6;
    config.tileSize = TileVoxelSize;
    config.borderSize = config.walkableRadius + 3;
    config.width = config.tileSize + config.borderSize * 2;
    config.height = config.tileSize + config.borderSize * 2;
    config.detailSampleDist = 3.f;
    config.detailSampleMaxError = 1.25f;

    config.bmin[0] = adt->MinX;
    config.bmin[1] = adt->MinY;
    config.bmin[2] = adt->MinZ;

    config.bmax[0] = adt->MaxX;
    config.bmax[1] = adt->MaxX;
    config.bmax[2] = adt->MaxX;

    rcHeightfield solid;
    ZERO(solid);

    rcContext ctx;

    if (!rcCreateHeightfield(&ctx, solid, config.width, config.height, config.bmin, config.bmax, config.cs, config.ch))
        return false;

    std::set<int> rasterizedWmos;
    std::set<int> rasterizedDoodads;

    // the mesh geometry can be rasterized into the height field stages, which is good for us
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x)
        {
            auto const chunk = adt->GetChunk(x, y);

            // adt terrain
            if (!!chunk->m_terrainVertices.size() && !!chunk->m_terrainIndices.size())
            {
                std::vector<float> chunkTerrainVertices;
                ConvertVerticesToRecast(chunk->m_terrainVertices, chunkTerrainVertices);

                std::vector<unsigned short> indices;
                ConvertToShort(chunk->m_terrainIndices, indices);

                std::vector<unsigned char> areas(indices.size() / 3, 0);

                if (!rcRasterizeTriangles(&ctx, &chunkTerrainVertices[0], chunk->m_terrainVertices.size(), &indices[0], &areas[0], indices.size() / 3, solid))
                    return false;
            }

            // liquid
            if (!!chunk->m_liquidVertices.size() && !!chunk->m_liquidIndices.size())
            {
                std::vector<float> chunkLiquidVertices;
                ConvertVerticesToRecast(chunk->m_liquidVertices, chunkLiquidVertices);

                std::vector<unsigned short> liquidIndices;
                ConvertToShort(chunk->m_liquidIndices, liquidIndices);

                std::vector<unsigned char> liquidAreas(liquidIndices.size() / 3, 1);

                if (!rcRasterizeTriangles(&ctx, &chunkLiquidVertices[0], chunk->m_liquidVertices.size(), &liquidIndices[0], &liquidAreas[0], liquidIndices.size() / 3, solid))
                    return false;
            }

            // wmos (and included doodads and liquid)
            for (auto const &wmoId : chunk->m_wmos)
            {
                if (rasterizedWmos.find(wmoId) != rasterizedWmos.end())
                    continue;

                auto const wmo = m_dataManager->m_continent->GetWmo(wmoId);

                assert(wmo);

                if (wmo->Vertices.size() && wmo->Indices.size())
                {
                    std::vector<float> wmoVertices;
                    ConvertVerticesToRecast(wmo->Vertices, wmoVertices);

                    std::vector<unsigned short> wmoIndices;
                    ConvertToShort(wmo->Indices, wmoIndices);

                    std::vector<unsigned char> wmoAreas(wmoIndices.size() / 3, 0);
                }
                //if (!rcRasterizeTriangles(&ctx, ))
            }

            // doodads
            for (auto const &doodadId : chunk->m_doodads)
            {
                if (rasterizedDoodads.find(doodadId) != rasterizedDoodads.end())
                    continue;

                auto const doodad = m_dataManager->m_continent->GetDoodad(doodadId);

                assert(doodad);
            }
        }

    return true;
}
}
}