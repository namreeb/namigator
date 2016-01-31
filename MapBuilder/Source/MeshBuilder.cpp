#include "MeshBuilder.hpp"

#include "parser/Include/parser.hpp"
#include "parser/Include/Adt/Adt.hpp"
#include "parser/Include/Adt/AdtChunk.hpp"

#include "utility/Include/MathHelper.hpp"
#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/AABBTree.hpp"

#include "RecastDetourBuild/Include/Common.hpp"

#include "Recast.h"
#include "DetourNavMeshBuilder.h"

#include <cassert>
#include <string>
#include <fstream>
#include <sstream>
#include <mutex>
#include <iomanip>
#include <vector>
#include <queue>

#define ZERO(x) memset(&x, 0, sizeof(x))

static_assert(sizeof(int) == sizeof(std::int32_t), "Recast requires 32 bit int type");
static_assert(sizeof(float) == 4, "float must be a 32 bit type");

namespace
{
bool Rasterize(rcContext &ctx, rcHeightfield &heightField, bool filterWalkable, float slope,
               const std::vector<utility::Vertex> &vertices, const std::vector<int> &indices, unsigned char areaFlags)
{
    if (!vertices.size() || !indices.size())
        return true;

    std::vector<float> rastVert;
    utility::Convert::VerticesToRecast(vertices, rastVert);

    std::vector<unsigned char> areas(indices.size() / 3, areaFlags);

    if (filterWalkable)
        rcClearUnwalkableTriangles(&ctx, slope, &rastVert[0], static_cast<int>(vertices.size()), &indices[0], static_cast<int>(indices.size() / 3), &areas[0]);

    return rcRasterizeTriangles(&ctx, &rastVert[0], static_cast<int>(vertices.size()), &indices[0], &areas[0], static_cast<int>(indices.size() / 3), heightField);
}

void FilterGroundBeneathLiquid(rcHeightfield &solid)
{
    for (int i = 0; i < solid.width*solid.height; ++i)
    {
        // as we go, we build a list of spans which will be removed in the event we find liquid
        std::vector<rcSpan *> spans;

        for (rcSpan *s = solid.spans[i]; s; s = s->next)
        {
            // if we found a non-wmo liquid span, remove everything beneath it
            if (!!(s->area & AreaFlags::Liquid) && !(s->area & AreaFlags::WMO))
            {
                for (auto ns : spans)
                    ns->area = RC_NULL_AREA;

                spans.clear();
            }
            // if we found a wmo liquid span, remove every wmo span beneath it
            else if (!!(s->area & (AreaFlags::Liquid | AreaFlags::WMO)))
            {
                for (auto ns : spans)
                    if (!!(ns->area & AreaFlags::WMO))
                        ns->area = RC_NULL_AREA;

                spans.clear();
            }
            else
                spans.push_back(s);
        }
    }
}

void RestoreAdtSpans(const std::vector<rcSpan *> &spans)
{
    for (auto s : spans)
        s->area |= AreaFlags::ADT;
}

// NOTE: this does not set bmin/bmax
void InitializeRecastConfig(rcConfig &config)
{
    ZERO(config);

    config.cs = RecastSettings::CellSize;
    config.ch = RecastSettings::CellHeight;
    config.walkableSlopeAngle = RecastSettings::WalkableSlope;
    config.walkableClimb = RecastSettings::VoxelWalkableClimb;
    config.walkableHeight = RecastSettings::VoxelWalkableHeight;
    config.walkableRadius = RecastSettings::VoxelWalkableRadius;
    config.maxEdgeLen = config.walkableRadius * 8;
    config.maxSimplificationError = RecastSettings::MaxSimplificationError;
    config.minRegionArea = RecastSettings::MinRegionSize;
    config.mergeRegionArea = RecastSettings::MergeRegionSize;
    config.maxVertsPerPoly = 6;
    config.tileSize = RecastSettings::TileVoxelSize;
    config.borderSize = config.walkableRadius + 3;
    config.width = config.tileSize + config.borderSize * 2;
    config.height = config.tileSize + config.borderSize * 2;
    config.detailSampleDist = 3.f;
    config.detailSampleMaxError = 1.25f;
}

using SmartHeightFieldPtr = std::unique_ptr<rcHeightfield, decltype(&rcFreeHeightField)>;
using SmartCompactHeightFieldPtr = std::unique_ptr<rcCompactHeightfield, decltype(&rcFreeCompactHeightfield)>;
using SmartContourSetPtr = std::unique_ptr<rcContourSet, decltype(&rcFreeContourSet)>;
using SmartPolyMeshPtr = std::unique_ptr<rcPolyMesh, decltype(&rcFreePolyMesh)>;
using SmartPolyMeshDetailPtr = std::unique_ptr<rcPolyMeshDetail, decltype(&rcFreePolyMeshDetail)>;

bool FinishMesh(rcContext &ctx, const rcConfig &config, int tileX, int tileY, const std::string &outputFile, rcHeightfield &solid)
{
    // initialize compact height field
    SmartCompactHeightFieldPtr chf(rcAllocCompactHeightfield(), rcFreeCompactHeightfield);

    if (!rcBuildCompactHeightfield(&ctx, config.walkableHeight, config.walkableClimb, solid, *chf))
        return false;

    //if (!rcErodeWalkableArea(&ctx, config.walkableRadius, *chf))
    //    return false;

    // we use watershed partitioning only for now.  we also have the option of monotone and partition layers.  see Sample_TileMesh.cpp for more information.

    if (!rcBuildDistanceField(&ctx, *chf))
        return false;

    if (!rcBuildRegions(&ctx, *chf, config.borderSize, config.minRegionArea, config.mergeRegionArea))
        return false;

    SmartContourSetPtr cset(rcAllocContourSet(), rcFreeContourSet);

    if (!rcBuildContours(&ctx, *chf, config.maxSimplificationError, config.maxEdgeLen, *cset))
        return false;

    assert(!!cset->nconts);

    SmartPolyMeshPtr polyMesh(rcAllocPolyMesh(), rcFreePolyMesh);

    if (!rcBuildPolyMesh(&ctx, *cset, config.maxVertsPerPoly, *polyMesh))
        return false;

    SmartPolyMeshDetailPtr polyMeshDetail(rcAllocPolyMeshDetail(), rcFreePolyMeshDetail);

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
    params.walkableHeight = RecastSettings::WalkableHeight;
    params.walkableRadius = RecastSettings::WalkableRadius;
    params.walkableClimb = 1.f;
    params.tileX = tileX;
    params.tileY = tileY;
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

    std::ofstream out(outputFile, std::ofstream::binary | std::ofstream::trunc);
    out.write(reinterpret_cast<const char *>(outData), outDataSize);
    out.close();

    dtFree(outData);

    return true;
}
}

MeshBuilder::MeshBuilder(const std::string &dataPath, const std::string &outputPath, const std::string &mapName) : m_outputPath(outputPath)
{
    parser::Parser::Initialize(dataPath.c_str());

    // this must follow the parser initialization
    m_map.reset(new parser::Map(mapName));

    memset(m_adtReferences, 0, sizeof(m_adtReferences));

    for (int y = 63; !!y; --y)
        for (int x = 63; !!x; --x)
        {
            if (!m_map->HasAdt(x, y))
                continue;

            AddReference(x - 1, y - 1); AddReference(x - 0, y - 1); AddReference(x + 1, y - 1);
            AddReference(x - 1, y - 0); AddReference(x - 0, y - 0); AddReference(x + 1, y - 0);
            AddReference(x - 1, y + 1); AddReference(x - 0, y + 1); AddReference(x + 1, y + 1);

            m_pendingAdts.push_back({ x, y });
        }
}

int MeshBuilder::AdtCount() const
{
    int ret = 0;

    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x)
            if (m_map->HasAdt(x, y))
                ++ret;

    return ret;
}

void MeshBuilder::SingleAdt(int adtX, int adtY)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    
    m_pendingAdts.clear();
    m_pendingAdts.push_back({ adtX, adtY });
}

bool MeshBuilder::GetNextAdt(int &adtX, int &adtY)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    if (m_pendingAdts.empty())
        return false;

    auto const front = m_pendingAdts.back();
    m_pendingAdts.pop_back();

    adtX = front.first;
    adtY = front.second;

    return true;
}

bool MeshBuilder::IsGlobalWMO() const
{
    return !!m_map->GetGlobalWmoInstance();
}

void MeshBuilder::AddReference(int adtX, int adtY)
{
    if (m_map->HasAdt(adtX, adtY))
        ++m_adtReferences[adtY][adtX];
}

void MeshBuilder::RemoveReference(int adtX, int adtY)
{
    if (!m_map->HasAdt(adtX, adtY))
        return;

    std::lock_guard<std::mutex> guard(m_mutex);
    --m_adtReferences[adtY][adtX];

    if (m_adtReferences[adtY][adtX] <= 0)
    {
#ifdef _DEBUG
        std::stringstream str;
        str << "No threads need ADT (" << std::setfill(' ') << std::setw(2) << adtX << ", "
            << std::setfill(' ') << std::setw(2) << adtY << ").  Unloading.\n";
        std::cout << str.str();
#endif
        auto const adt = m_map->GetAdt(adtX, adtY);

        // check if there are any wmos or doodads we can now remove
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 16; ++x)
            {
                auto const chunk = adt->GetChunk(x, y);
                
                for (auto const &wmoId : chunk->m_wmos)
                {
                    auto const wmo = m_map->GetWmoInstance(wmoId);

                    if (!wmo)
                        continue;

                    bool unload = true;

                    for (auto const &a : wmo->Adts)
                        if (m_adtReferences[a.second][a.first] > 0)
                        {
                            unload = false;
                            break;
                        }

                    if (unload)
                        m_map->UnloadWmoInstance(wmoId);
                }

                for (auto const &doodadId : chunk->m_doodads)
                {
                    auto const doodad = m_map->GetDoodadInstance(doodadId);

                    if (!doodad)
                        continue;

                    bool unload = true;

                    for (auto const &a : doodad->Adts)
                        if (m_adtReferences[a.second][a.first] > 0)
                        {
                            unload = false;
                            break;
                        }

                    if (unload)
                        m_map->UnloadDoodadInstance(doodadId);
                }
            }

        m_map->UnloadAdt(adtX, adtY);
    }
}

bool MeshBuilder::GenerateAndSaveGlobalWMO()
{
    auto const wmoInstance = m_map->GetGlobalWmoInstance();

    assert(!!wmoInstance);

//#ifdef _DEBUG
//    wmo->WriteGlobalObjFile(m_Map->Name);
//#endif

    rcConfig config;
    InitializeRecastConfig(config);

    config.bmin[0] = -wmoInstance->Bounds.MaxCorner.Y;
    config.bmin[1] =  wmoInstance->Bounds.MinCorner.Z;
    config.bmin[2] = -wmoInstance->Bounds.MaxCorner.X;

    config.bmax[0] = -wmoInstance->Bounds.MinCorner.Y;
    config.bmax[1] =  wmoInstance->Bounds.MaxCorner.Z;
    config.bmax[2] = -wmoInstance->Bounds.MinCorner.X;

    config.bmin[0] -= config.borderSize * config.cs;
    config.bmin[2] -= config.borderSize * config.cs;
    config.bmax[0] += config.borderSize * config.cs;
    config.bmax[2] += config.borderSize * config.cs;

    rcContext ctx;

    SmartHeightFieldPtr solid(rcAllocHeightfield(), rcFreeHeightField);

    if (!rcCreateHeightfield(&ctx, *solid, config.width, config.height, config.bmin, config.bmax, config.cs, config.ch))
        return false;

    std::vector<utility::Vertex> vertices;
    std::vector<int> indices;

    // wmo terrain
    wmoInstance->BuildTriangles(vertices, indices);
    if (!Rasterize(ctx, *solid, true, config.walkableSlopeAngle, vertices, indices, AreaFlags::WMO))
        return false;

    // wmo liquid
    wmoInstance->BuildLiquidTriangles(vertices, indices);
    if (!Rasterize(ctx, *solid, false, config.walkableSlopeAngle, vertices, indices, AreaFlags::WMO | AreaFlags::Liquid))
        return false;

    // wmo doodads
    wmoInstance->BuildDoodadTriangles(vertices, indices);
    if (!Rasterize(ctx, *solid, true, config.walkableSlopeAngle, vertices, indices, AreaFlags::WMO | AreaFlags::Doodad))
        return false;

    FilterGroundBeneathLiquid(*solid);

    // note that no area id preservation is necessary here because we have no ADT terrain
    rcFilterLowHangingWalkableObstacles(&ctx, config.walkableClimb, *solid);
    rcFilterLedgeSpans(&ctx, config.walkableHeight, config.walkableClimb, *solid);
    rcFilterWalkableLowHeightSpans(&ctx, config.walkableHeight, *solid);

    std::stringstream str;

    str << m_outputPath << "\\" << m_map->Name <<  ".map";

    return FinishMesh(ctx, config, 0, 0, str.str(), *solid);
}

bool MeshBuilder::GenerateAndSaveTile(int adtX, int adtY)
{
    const parser::Adt *adts[9] = {
        m_map->GetAdt(adtX - 1, adtY - 1), m_map->GetAdt(adtX - 0, adtY - 1), m_map->GetAdt(adtX + 1, adtY - 1),
        m_map->GetAdt(adtX - 1, adtY - 0), m_map->GetAdt(adtX - 0, adtY - 0), m_map->GetAdt(adtX + 1, adtY - 0),
        m_map->GetAdt(adtX - 1, adtY + 1), m_map->GetAdt(adtX - 0, adtY + 1), m_map->GetAdt(adtX + 1, adtY + 1),
    };

    const parser::Adt *thisTile = adts[4];

    assert(!!thisTile);

//#ifdef _DEBUG
//    thisTile->WriteObjFile();
//#endif

    rcConfig config;
    InitializeRecastConfig(config);

    config.bmin[0] = -thisTile->Bounds.MaxCorner.Y;
    config.bmin[1] =  thisTile->Bounds.MinCorner.Z;
    config.bmin[2] = -thisTile->Bounds.MaxCorner.X;

    config.bmax[0] = -thisTile->Bounds.MinCorner.Y;
    config.bmax[1] =  thisTile->Bounds.MaxCorner.Z;
    config.bmax[2] = -thisTile->Bounds.MinCorner.X;

    config.bmin[0] -= config.borderSize * config.cs;
    config.bmin[2] -= config.borderSize * config.cs;
    config.bmax[0] += config.borderSize * config.cs;
    config.bmax[2] += config.borderSize * config.cs;

    rcContext ctx;

    SmartHeightFieldPtr solid(rcAllocHeightfield(), rcFreeHeightField);

    if (!rcCreateHeightfield(&ctx, *solid, config.width, config.height, config.bmin, config.bmax, config.cs, config.ch))
        return false;

    std::set<int> rasterizedWmos;
    std::set<int> rasterizedDoodads;

    for (int i = 0; i < sizeof(adts) / sizeof(adts[0]); ++i)
    {
        if (!adts[i])
            continue;

        // the mesh geometry can be rasterized into the height field stages, which is good for us
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 16; ++x)
            {
                auto const chunk = adts[i]->GetChunk(x, y);

                // adt terrain
                if (!Rasterize(ctx, *solid, false, config.walkableSlopeAngle, chunk->m_terrainVertices, chunk->m_terrainIndices, AreaFlags::ADT))
                    return false;

                // liquid
                if (!Rasterize(ctx, *solid, false, config.walkableSlopeAngle, chunk->m_liquidVertices, chunk->m_liquidIndices, AreaFlags::Liquid))
                    return false;

                // wmos (and included doodads and liquid)
                for (auto const &wmoId : chunk->m_wmos)
                {
                    if (rasterizedWmos.find(wmoId) != rasterizedWmos.end())
                        continue;

                    auto const wmoInstance = m_map->GetWmoInstance(wmoId);

                    if (!wmoInstance)
                    {
                        std::stringstream str;
                        str << "Could not find required WMO ID = " << wmoId << " needed by ADT (" << adtX << ", " << adtY << ")\n";
                        std::cout << str.str();
                    }

                    assert(!!wmoInstance);

                    std::vector<utility::Vertex> vertices;
                    std::vector<int> indices;

                    wmoInstance->BuildTriangles(vertices, indices);
                    if (!Rasterize(ctx, *solid, true, config.walkableSlopeAngle, vertices, indices, AreaFlags::WMO))
                        return false;

                    wmoInstance->BuildLiquidTriangles(vertices, indices);
                    if (!Rasterize(ctx, *solid, false, config.walkableSlopeAngle, vertices, indices, AreaFlags::WMO | AreaFlags::Liquid))
                        return false;

                    wmoInstance->BuildDoodadTriangles(vertices, indices);
                    if (!Rasterize(ctx, *solid, true, config.walkableSlopeAngle, vertices, indices, AreaFlags::WMO | AreaFlags::Doodad))
                        return false;

                    rasterizedWmos.insert(wmoId);
                }

                // doodads
                for (auto const &doodadId : chunk->m_doodads)
                {
                    if (rasterizedDoodads.find(doodadId) != rasterizedDoodads.end())
                        continue;

                    auto const doodadInstance = m_map->GetDoodadInstance(doodadId);

                    assert(!!doodadInstance);

                    std::vector<utility::Vertex> vertices;
                    std::vector<int> indices;

                    doodadInstance->BuildTriangles(vertices, indices);
                    if (!Rasterize(ctx, *solid, true, config.walkableSlopeAngle, vertices, indices, AreaFlags::Doodad))
                        return false;

                    rasterizedDoodads.insert(doodadId);
                }
            }
    }

    FilterGroundBeneathLiquid(*solid);

    // save all span area flags because we dont want the upcoming filtering to apply to ADT terrain
    {
        std::vector<rcSpan *> adtSpans;
                
        adtSpans.reserve(solid->width*solid->height);

        for (int i = 0; i < solid->width * solid->height; ++i)
            for (rcSpan *s = solid->spans[i]; s; s = s->next)
                if (!!(s->area & AreaFlags::ADT))
                    adtSpans.push_back(s);

        rcFilterLowHangingWalkableObstacles(&ctx, config.walkableClimb, *solid);

        RestoreAdtSpans(adtSpans);

        rcFilterLedgeSpans(&ctx, config.walkableHeight, config.walkableClimb, *solid);

        RestoreAdtSpans(adtSpans);

        rcFilterWalkableLowHeightSpans(&ctx, config.walkableHeight, *solid);

        RestoreAdtSpans(adtSpans);
    }

    // Write the BVH for every new WMO
    for (auto const& wmoId : rasterizedWmos)
    {
        auto const wmo = m_map->GetWmoInstance(wmoId)->Model;

        std::lock_guard<std::mutex> guard(m_mutex);

        if (m_bvhWmos.find(wmo->FullPath) != m_bvhWmos.end())
            continue;

        utility::AABBTree aabbTree(wmo->Vertices, wmo->Indices);

        std::stringstream out;
        out << m_outputPath << "\\BVH\\WMO_" << wmo->FileName << ".bvh";

        std::ofstream o(out.str(), std::ofstream::binary);
        aabbTree.Serialize(o);
        o.close();

        m_bvhWmos.insert(wmo->FullPath);

        // Also write BVH for any WMO-spawned doodads
        for (auto const &doodadSet : wmo->DoodadSets)
        {
            for (auto const &wmoDoodad : doodadSet)
            {
                auto const doodad = wmoDoodad->Parent;

                if (m_bvhDoodads.find(doodad->Name) != m_bvhDoodads.end())
                    continue;

                utility::AABBTree doodadTree(doodad->Vertices, doodad->Indices);
                
                std::stringstream dout;
                dout << m_outputPath << "\\BVH\\Doodad_" << doodad->Name << ".bvh";
                std::ofstream doodadOut(dout.str(), std::ofstream::binary);
                doodadTree.Serialize(doodadOut);
                doodadOut.close();

                m_bvhDoodads.insert(doodad->Name);
            }
        }
    }

    // Write the BVH for every new doodad
    for (auto const& doodadId : rasterizedDoodads)
    {
        auto const doodad = m_map->GetDoodadInstance(doodadId)->Model;

        std::lock_guard<std::mutex> guard(m_mutex);

        if (m_bvhDoodads.find(doodad->Name) != m_bvhDoodads.end())
            continue;

        utility::AABBTree aabbTree(doodad->Vertices, doodad->Indices);

        std::stringstream out;
        out << m_outputPath << "\\BVH\\Doodad_" << doodadId << ".bvh";

        std::ofstream o(out.str(), std::ofstream::binary);
        aabbTree.Serialize(o);
        o.close();

        m_bvhDoodads.insert(doodad->Name);
    }

    std::stringstream str;

    str << m_outputPath << "\\" << m_map->Name << "_" << adtX << "_" << adtY << ".map";

    return FinishMesh(ctx, config, adtX, adtY, str.str(), *solid);
}

#ifdef _DEBUG
std::string MeshBuilder::AdtMap() const
{
    std::lock_guard<std::mutex> guard(m_mutex);

    std::stringstream ret;

    for (int y = 1; y < 64; ++y)
    {
        for (int x = 1; x < 64; ++x)
        {
            if (!m_map->HasAdt(x, y))
            {
                ret << " ";
                continue;
            }

            bool pending = false;

            for (auto const &adt : m_pendingAdts)
                if (adt.first == x && adt.second == y)
                {
                    pending = true;
                    break;
                }

            ret << (pending ? "X" : ".");
        }

        ret << "\n";
    }

    return ret.str();
}

std::string MeshBuilder::AdtReferencesMap() const
{
    std::lock_guard<std::mutex> guard(m_mutex);

    std::stringstream ret;

    for (int y = 1; y < 64; ++y)
    {
        for (int x = 1; x < 64; ++x)
        {
            if (m_map->HasAdt(x, y))
                ret << m_adtReferences[y][x];
            else
                ret << " ";
        }

        ret << "\n";
    }

    return ret.str();
}

std::string MeshBuilder::WmoMap() const
{
    std::lock_guard<std::mutex> guard(m_mutex);

    std::stringstream ret;

    for (int y = 1; y < 64; ++y)
    {
        for (int x = 1; x < 64; ++x)
        {
            if (!m_map->HasAdt(x, y) || !m_map->IsAdtLoaded(x, y))
            {
                ret << " ";
                continue;
            }

            auto const adt = m_map->GetAdt(x, y);
            assert(!!adt);

            auto const count = adt->GetWmoCount();

            if (count > 9)
                ret << "X";
            else
                ret << count;
        }

        ret << "\n";
    }

    return ret.str();
}

void MeshBuilder::WriteMemoryUsage(std::ostream &o) const
{
    std::lock_guard<std::mutex> guard(m_mutex);
    o << "ADTs remaining: " << m_pendingAdts.size() << std::endl;

    m_map->WriteMemoryUsage(o);
}
#endif