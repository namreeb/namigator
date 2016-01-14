#include "MeshBuilder.hpp"
#include "AreaFlags.hpp"

#include "utility/Include/MathHelper.hpp"

#include "Recast.h"
#include "DetourNavMeshBuilder.h"

#include <cassert>
#include <string>
#include <fstream>
#include <sstream>

#define ZERO(x) memset(&x, 0, sizeof(x));

namespace pathfind
{
namespace build
{
namespace
{
bool Rasterize(rcContext &ctx, rcHeightfield &heightField, bool filterWalkable, float slope,
               const std::vector<utility::Vertex> &vertices, const std::vector<int> &indices, unsigned char areaFlags)
{
    if (!vertices.size() || !indices.size())
        return true;

    std::vector<float> rastVert;
    utility::Convert::VerticesToRecast(vertices, rastVert);

    std::vector<unsigned short> rastIndices;
    utility::Convert::ToShort(indices, rastIndices);

    std::vector<unsigned char> areas(indices.size() / 3, areaFlags);

    // XXX FIXME - why on earth does recast take indices as ints here, but unsigned short elsewhere? o.O
    if (filterWalkable)
        rcClearUnwalkableTriangles(&ctx, slope, &rastVert[0], vertices.size(), &indices[0], indices.size() / 3, &areas[0]);

    return rcRasterizeTriangles(&ctx, &rastVert[0], vertices.size(), &rastIndices[0], &areas[0], rastIndices.size() / 3, heightField);
}
}

MeshBuilder::MeshBuilder(DataManager *dataManager) : m_dataManager(dataManager) {}

// todo: filter out terrain triangles underneath liquid
bool MeshBuilder::GenerateAndSaveTile(int adtX, int adtY)
{
    auto const adt = m_dataManager->m_continent->LoadAdt(adtX, adtY);

    assert(adt);

#ifdef _DEBUG
    adt->WriteObjFile();
#endif

    rcConfig config;
    ZERO(config);

    config.cs = TileSize / static_cast<float>(TileVoxelSize);
    config.ch = CellHeight;
    config.walkableSlopeAngle = WalkableSlope;
    config.walkableClimb = static_cast<int>(std::round(WalkableClimb / CellHeight));
    config.walkableHeight = static_cast<int>(std::round(WalkableHeight / CellHeight));
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

    config.bmin[0] = -adt->MaxY;
    config.bmin[1] =  adt->MinZ;
    config.bmin[2] = -adt->MaxX;

    config.bmax[0] = -adt->MinY;
    config.bmax[1] =  adt->MaxZ;
    config.bmax[2] = -adt->MinX;

    rcContext ctx;

    std::unique_ptr<rcHeightfield, decltype(&rcFreeHeightField)> solid(rcAllocHeightfield(), rcFreeHeightField);

    if (!rcCreateHeightfield(&ctx, *solid, config.width, config.height, config.bmin, config.bmax, config.cs, config.ch))
        return false;

    std::set<int> rasterizedWmos;
    std::set<int> rasterizedDoodads;

    // the mesh geometry can be rasterized into the height field stages, which is good for us
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x)
        {
            auto const chunk = adt->GetChunk(x, y);

            // adt terrain
            if (!Rasterize(ctx, *solid, false, config.walkableSlopeAngle, chunk->m_terrainVertices, chunk->m_terrainIndices, AreaFlags::Adt))
                return false;

            // liquid
            if (!Rasterize(ctx, *solid, false, config.walkableSlopeAngle, chunk->m_liquidVertices, chunk->m_liquidIndices, AreaFlags::Liquid))
                return false;

            // wmos (and included doodads and liquid)
            for (auto const &wmoId : chunk->m_wmos)
            {
                if (rasterizedWmos.find(wmoId) != rasterizedWmos.end())
                    continue;

                auto const wmo = m_dataManager->m_continent->GetWmo(wmoId);

                assert(wmo);

                if (!Rasterize(ctx, *solid, true, config.walkableSlopeAngle, wmo->Vertices, wmo->Indices, AreaFlags::Wmo))
                    return false;

                if (!Rasterize(ctx, *solid, false, config.walkableSlopeAngle, wmo->LiquidVertices, wmo->LiquidIndices, AreaFlags::Wmo | AreaFlags::Liquid))
                    return false;

                if (!Rasterize(ctx, *solid, true, config.walkableSlopeAngle, wmo->DoodadVertices, wmo->DoodadIndices, AreaFlags::Wmo | AreaFlags::Doodad))
                    return false;
            }

            // doodads
            for (auto const &doodadId : chunk->m_doodads)
            {
                if (rasterizedDoodads.find(doodadId) != rasterizedDoodads.end())
                    continue;

                auto const doodad = m_dataManager->m_continent->GetDoodad(doodadId);

                assert(doodad);

                if (!Rasterize(ctx, *solid, true, config.walkableSlopeAngle, doodad->Vertices, doodad->Indices, AreaFlags::Doodad))
                    return false;
            }
        }

    rcFilterLowHangingWalkableObstacles(&ctx, config.walkableClimb, *solid);
    rcFilterLedgeSpans(&ctx, config.walkableHeight, config.walkableClimb, *solid);
    rcFilterWalkableLowHeightSpans(&ctx, config.walkableHeight, *solid);

    // initialize compact height field
    std::unique_ptr<rcCompactHeightfield, decltype(&rcFreeCompactHeightfield)> chf(rcAllocCompactHeightfield(), rcFreeCompactHeightfield);

    if (!rcBuildCompactHeightfield(&ctx, config.walkableHeight, config.walkableClimb, *solid, *chf))
        return false;

    solid.reset(nullptr);

    if (!rcErodeWalkableArea(&ctx, config.walkableRadius, *chf))
        return false;
    
    // any further area marking is done here.  not sure if we will need this or not

    // we use watershed partitioning only for now.  we also have the option of monotone and partition layers.  see Sample_TileMesh.cpp for more information.

    if (!rcBuildDistanceField(&ctx, *chf))
        return false;

    if (!rcBuildRegions(&ctx, *chf, config.borderSize, config.minRegionArea, config.mergeRegionArea))
        return false;

    std::unique_ptr<rcContourSet, decltype(&rcFreeContourSet)> cset(rcAllocContourSet(), rcFreeContourSet);

    if (!rcBuildContours(&ctx, *chf, config.maxSimplificationError, config.maxEdgeLen, *cset))
        return false;

    assert(!!cset->nconts);

    std::unique_ptr<rcPolyMesh, decltype(&rcFreePolyMesh)> polyMesh(rcAllocPolyMesh(), rcFreePolyMesh);

    if (!rcBuildPolyMesh(&ctx, *cset, config.maxVertsPerPoly, *polyMesh))
        return false;

    std::unique_ptr<rcPolyMeshDetail, decltype(&rcFreePolyMeshDetail)> polyMeshDetail(rcAllocPolyMeshDetail(), rcFreePolyMeshDetail);

    if (!rcBuildPolyMeshDetail(&ctx, *polyMesh, *chf, config.detailSampleDist, config.detailSampleMaxError, *polyMeshDetail))
        return false;

    chf.reset(nullptr);
    cset.reset(nullptr);

    dtNavMeshCreateParams params;
    ZERO(params);

    params.verts = polyMesh->verts;
    params.vertCount = polyMesh->nverts;
    params.polys = polyMesh->polys;
    params.polyAreas = polyMesh->areas;
    params.polyFlags = polyMesh->flags;
    params.polyCount = polyMesh->npolys;
    params.nvp = polyMesh->nvp;
    params.detailMeshes = polyMeshDetail->meshes;
    params.detailVerts = polyMeshDetail->verts;
    params.detailVertsCount = polyMeshDetail->nverts;
    params.detailTris = polyMeshDetail->tris;
    params.detailTriCount = polyMeshDetail->ntris;
    params.walkableHeight = WalkableHeight;
    params.walkableRadius = WalkableRadius;
    params.walkableClimb = 1.f;
    params.tileX = adtX;
    params.tileY = adtY;
    params.tileLayer = 0;
    memcpy(params.bmin, polyMesh->bmin, sizeof(polyMesh->bmin));
    memcpy(params.bmax, polyMesh->bmax, sizeof(polyMesh->bmax));
    params.cs = config.cs;
    params.ch = config.ch;
    params.buildBvTree = true;

    unsigned char *outData;
    int outDataSize;
    if (!dtCreateNavMeshData(&params, &outData, &outDataSize))
        return false;

    std::stringstream str;

    str << m_dataManager->m_outputPath << "\\" << m_dataManager->m_continent->Name << "_" << adtX << "_" << adtY << ".map";

    std::ofstream out(str.str(), std::ofstream::binary|std::ofstream::trunc);
    out.write(reinterpret_cast<const char *>(outData), outDataSize);
    out.close();

    dtFree(outData);

    return true;
}
}
}