#include "MeshBuilder.hpp"

#include "parser/Include/parser.hpp"
#include "parser/Include/Adt/Adt.hpp"
#include "parser/Include/Adt/AdtChunk.hpp"

#include "utility/Include/MathHelper.hpp"
#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/AABBTree.hpp"
#include "utility/Include/Directory.hpp"

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
#include <unordered_set>

#define ZERO(x) memset(&x, 0, sizeof(x))

static_assert(sizeof(int) == sizeof(std::int32_t), "Recast requires 32 bit int type");
static_assert(sizeof(float) == 4, "float must be a 32 bit type");

//#define DISABLE_SELECTIVE_FILTERING

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
    config.maxEdgeLen = config.walkableRadius * 4;
    config.maxSimplificationError = RecastSettings::MaxSimplificationError;
    config.minRegionArea = RecastSettings::MinRegionSize*RecastSettings::MinRegionSize;
    config.mergeRegionArea = RecastSettings::MergeRegionSize*RecastSettings::MergeRegionSize;
    config.maxVertsPerPoly = 6;
    config.tileSize = RecastSettings::TileVoxelSize;
    config.borderSize = config.walkableRadius + 3;
    config.width = config.tileSize + config.borderSize * 2;
    config.height = config.tileSize + config.borderSize * 2;
    config.detailSampleDist = 3.f;
    config.detailSampleMaxError = 0.75f;
}

using SmartHeightFieldPtr = std::unique_ptr<rcHeightfield, decltype(&rcFreeHeightField)>;
using SmartCompactHeightFieldPtr = std::unique_ptr<rcCompactHeightfield, decltype(&rcFreeCompactHeightfield)>;
using SmartContourSetPtr = std::unique_ptr<rcContourSet, decltype(&rcFreeContourSet)>;
using SmartPolyMeshPtr = std::unique_ptr<rcPolyMesh, decltype(&rcFreePolyMesh)>;
using SmartPolyMeshDetailPtr = std::unique_ptr<rcPolyMeshDetail, decltype(&rcFreePolyMeshDetail)>;

bool FinishMesh(rcContext &ctx, const rcConfig &config, int tileX, int tileY, std::ofstream &out, rcHeightfield &solid)
{
    // initialize compact height field
    SmartCompactHeightFieldPtr chf(rcAllocCompactHeightfield(), rcFreeCompactHeightfield);

    if (!rcBuildCompactHeightfield(&ctx, config.walkableHeight, config.walkableClimb, solid, *chf))
        return false;

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

    // too many vertices?
    if (polyMesh->nverts >= 0xFFFF)
        return false;

    for (int i = 0; i < polyMesh->npolys; ++i)
    {
        if (!polyMesh->areas[i])
            continue;

        polyMesh->flags[i] = static_cast<unsigned short>(PolyFlags::Walkable | polyMesh->areas[i]);
    }

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

    const std::int32_t outSize = static_cast<std::int32_t>(outDataSize);
    out.write(reinterpret_cast<const char *>(&outSize), sizeof(outSize));
    out.write(reinterpret_cast<const char *>(outData), outDataSize);

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

    utility::Directory::Create(m_outputPath + "\\Nav\\" + mapName);
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

void MeshBuilder::SerializeWmo(const parser::Wmo *wmo)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    if (m_bvhWmos.find(wmo->FileName) != m_bvhWmos.end())
        return;

    utility::AABBTree aabbTree(wmo->Vertices, wmo->Indices);

    std::stringstream out;
    out << m_outputPath << "\\BVH\\WMO_" << wmo->FileName << ".bvh";

    std::ofstream o(out.str(), std::ofstream::binary|std::ofstream::trunc);
    aabbTree.Serialize(o);

    m_bvhWmos.insert(wmo->FileName);

    const std::uint32_t doodadSetCount = static_cast<std::uint32_t>(wmo->DoodadSets.size());
    o.write(reinterpret_cast<const char *>(&doodadSetCount), sizeof(doodadSetCount));

    // Also write BVH for any WMO-spawned doodads
    for (auto const &doodadSet : wmo->DoodadSets)
    {
        const std::uint32_t doodadSetSize = static_cast<std::uint32_t>(doodadSet.size());
        o.write(reinterpret_cast<const char *>(&doodadSetSize), sizeof(doodadSetSize));

        for (auto const &wmoDoodad : doodadSet)
        {
            auto const doodad = wmoDoodad->Parent;

            o << wmoDoodad->TransformMatrix;
            o << wmoDoodad->Bounds;
            o << std::left << std::setw(64) << std::setfill('\000') << doodad->FileName;

            SerializeDoodad(doodad);
        }
    }
}

void MeshBuilder::SerializeDoodad(const parser::Doodad *doodad)
{
    if (m_bvhDoodads.find(doodad->FileName) != m_bvhDoodads.end())
        return;

    utility::AABBTree doodadTree(doodad->Vertices, doodad->Indices);

    std::stringstream dout;
    dout << m_outputPath << "\\BVH\\Doodad_" << doodad->FileName << ".bvh";
    std::ofstream doodadOut(dout.str(), std::ofstream::binary|std::ofstream::trunc);
    doodadTree.Serialize(doodadOut);

    m_bvhDoodads.insert(doodad->FileName);
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

        m_map->UnloadAdt(adtX, adtY);
    }
}

bool MeshBuilder::GenerateAndSaveGlobalWMO()
{
    auto const wmoInstance = m_map->GetGlobalWmoInstance();

    assert(!!wmoInstance);

    SerializeWmo(wmoInstance->Model);

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
    str << m_outputPath << "\\Nav\\" << m_map->Name << "\\" << m_map->Name << ".nav";

    std::ofstream out(str.str(), std::ofstream::binary|std::ofstream::trunc);

    return FinishMesh(ctx, config, 0, 0, out, *solid);
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

    std::unordered_set<unsigned int> rasterizedWmos;
    std::unordered_set<unsigned int> rasterizedDoodads;

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
            }

        // wmos (and included doodads and liquid)
        for (auto const &wmoId : adts[i]->WmoInstances)
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
        for (auto const &doodadId : adts[i]->DoodadInstances)
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

    FilterGroundBeneathLiquid(*solid);

#ifndef DISABLE_SELECTIVE_FILTERING
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
#else
    rcFilterLowHangingWalkableObstacles(&ctx, config.walkableClimb, *solid);
    rcFilterLedgeSpans(&ctx, config.walkableHeight, config.walkableClimb, *solid);
    rcFilterWalkableLowHeightSpans(&ctx, config.walkableHeight, *solid);
#endif

    // Write the BVH for every new WMO
    for (auto const& wmoId : rasterizedWmos)
        SerializeWmo(m_map->GetWmoInstance(wmoId)->Model);

    {
        std::lock_guard<std::mutex> guard(m_mutex);

        // Write the BVH for every new doodad
        for (auto const& doodadId : rasterizedDoodads)
            SerializeDoodad(m_map->GetDoodadInstance(doodadId)->Model);
    }

    std::stringstream str;
    str << m_outputPath << "\\Nav\\" << m_map->Name << "\\" << std::setw(2) << std::setfill('0') << adtX << "_" << std::setw(2) << std::setfill('0') << adtY << ".nav";

    std::ofstream out(str.str(), std::ofstream::binary|std::ofstream::trunc);

    const std::uint32_t wmoInstanceCount = static_cast<std::uint32_t>(thisTile->WmoInstances.size());
    out.write(reinterpret_cast<const char *>(&wmoInstanceCount), sizeof(wmoInstanceCount));

    for (auto const &wmoId : thisTile->WmoInstances)
    {
        const std::uint32_t id32 = static_cast<std::uint32_t>(wmoId);
        out.write(reinterpret_cast<const char *>(&id32), sizeof(id32));
    }

    const std::uint32_t doodadInstanceCount = static_cast<std::uint32_t>(thisTile->DoodadInstances.size());
    out.write(reinterpret_cast<const char *>(&doodadInstanceCount), sizeof(doodadInstanceCount));

    for (auto const &doodadId : thisTile->DoodadInstances)
    {
        const std::uint32_t id32 = static_cast<std::uint32_t>(doodadId);
        out.write(reinterpret_cast<const char *>(&id32), sizeof(id32));
    }

    return FinishMesh(ctx, config, adtX, adtY, out, *solid);
}

bool MeshBuilder::GenerateAndSaveGSet()
{
    auto const wmoInstance = m_map->GetGlobalWmoInstance();

    assert(!!wmoInstance);

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

    std::vector<utility::Vertex> vertices;
    std::vector<float> recastVertices;
    std::vector<int> indices;

    // wmo terrain
    wmoInstance->BuildTriangles(vertices, indices);
    utility::Convert::VerticesToRecast(vertices, recastVertices);

    assert(recastVertices.size() == vertices.size() * 3);

    {
        std::ofstream gsetOut(m_map->Name + ".gset", std::ofstream::trunc);

        gsetOut << "s " << config.cs << " " << config.ch << " " << RecastSettings::WalkableHeight << " " << RecastSettings::WalkableRadius << " " << RecastSettings::WalkableClimb << " "
                << RecastSettings::WalkableSlope << " " << config.minRegionArea << " " << config.mergeRegionArea << " " << (config.cs * config.maxEdgeLen) << " " << config.maxSimplificationError << " "
                << config.maxVertsPerPoly << " " << config.detailSampleDist << " " << config.detailSampleMaxError << " " << 0 << " "
                << config.bmin[0] << " " << config.bmin[1] << " " << config.bmin[2] << " "
                << config.bmax[0] << " " << config.bmax[1] << " " << config.bmax[2] << " " << config.tileSize << std::endl;

        gsetOut << "f " << m_map->Name << ".obj" << std::endl;
    }

    int indexOffset = 1;

    std::ofstream objOut(m_map->Name + ".obj", std::ofstream::trunc);

    objOut << "# wmo terrain" << std::endl;
    for (size_t i = 0; i < vertices.size(); ++i)
        objOut << "v " << recastVertices[i * 3 + 0] << " " << recastVertices[i * 3 + 1] << " " << recastVertices[i * 3 + 2] << std::endl;
    for (size_t i = 0; i < indices.size(); i += 3)
        objOut << "f " << (indexOffset + indices[i + 0]) << " " << (indexOffset + indices[i + 1]) << " " << (indexOffset + indices[i + 2]) << std::endl;

    indexOffset += static_cast<int>(vertices.size());

    // wmo liquid
    wmoInstance->BuildLiquidTriangles(vertices, indices);
    utility::Convert::VerticesToRecast(vertices, recastVertices);

    if (vertices.size() && indices.size())
    {
        objOut << "# wmo liquid" << std::endl;
        for (size_t i = 0; i < vertices.size(); ++i)
            objOut << "v " << recastVertices[i * 3 + 0] << " " << recastVertices[i * 3 + 1] << " " << recastVertices[i * 3 + 2] << std::endl;
        for (size_t i = 0; i < indices.size(); i += 3)
            objOut << "f " << (indexOffset + indices[i + 0]) << " " << (indexOffset + indices[i + 1]) << " " << (indexOffset + indices[i + 2]) << std::endl;

        indexOffset += static_cast<int>(vertices.size());
    }

    // wmo doodads
    wmoInstance->BuildDoodadTriangles(vertices, indices);
    utility::Convert::VerticesToRecast(vertices, recastVertices);

    if (vertices.size() && indices.size())
    {
        objOut << "# wmo doodads" << std::endl;
        for (size_t i = 0; i < vertices.size(); ++i)
            objOut << "v " << recastVertices[i * 3 + 0] << " " << recastVertices[i * 3 + 1] << " " << recastVertices[i * 3 + 2] << std::endl;
        for (size_t i = 0; i < indices.size(); i += 3)
            objOut << "f " << (indexOffset + indices[i + 0]) << " " << (indexOffset + indices[i + 1]) << " " << (indexOffset + indices[i + 2]) << std::endl;
    }

    return true;
}

bool MeshBuilder::GenerateAndSaveGSet(int adtX, int adtY)
{
    auto const tile = m_map->GetAdt(adtX - 0, adtY - 0);

    rcConfig config;
    InitializeRecastConfig(config);

    config.bmin[0] = -tile->Bounds.MaxCorner.Y;
    config.bmin[1] =  tile->Bounds.MinCorner.Z;
    config.bmin[2] = -tile->Bounds.MaxCorner.X;

    config.bmax[0] = -tile->Bounds.MinCorner.Y;
    config.bmax[1] =  tile->Bounds.MaxCorner.Z;
    config.bmax[2] = -tile->Bounds.MinCorner.X;

    config.bmin[0] -= config.borderSize * config.cs;
    config.bmin[2] -= config.borderSize * config.cs;
    config.bmax[0] += config.borderSize * config.cs;
    config.bmax[2] += config.borderSize * config.cs;

    auto const filename = m_map->Name + "_" + std::to_string(adtX) + "_" + std::to_string(adtY);

    {
        std::ofstream gsetOut(filename + ".gset", std::ofstream::trunc);

        gsetOut << "s " << config.cs << " " << config.ch << " " << RecastSettings::WalkableHeight << " " << RecastSettings::WalkableRadius << " " << RecastSettings::WalkableClimb << " "
                << RecastSettings::WalkableSlope << " " << config.minRegionArea << " " << config.mergeRegionArea << " " << (config.cs * config.maxEdgeLen) << " " << config.maxSimplificationError << " "
                << config.maxVertsPerPoly << " " << config.detailSampleDist << " " << config.detailSampleMaxError << " " << 0 << " "
                << config.bmin[0] << " " << config.bmin[1] << " " << config.bmin[2] << " "
                << config.bmax[0] << " " << config.bmax[1] << " " << config.bmax[2] << " " << config.tileSize << std::endl;

        gsetOut << "f " << filename << ".obj" << std::endl;
    }

    std::ofstream objOut(filename + ".obj", std::ofstream::trunc);

    int indexOffset = 1;

    // the mesh geometry can be rasterized into the height field stages, which is good for us
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x)
        {
            auto const chunk = tile->GetChunk(x, y);

            std::vector<float> recastVertices;
            utility::Convert::VerticesToRecast(chunk->m_terrainVertices, recastVertices);

            if (recastVertices.size() && chunk->m_terrainIndices.size())
            {
                objOut << "# ADT terrain" << std::endl;
                for (size_t i = 0; i < recastVertices.size(); i += 3)
                    objOut << "v " << recastVertices[i + 0] << " " << recastVertices[i + 1] << " " << recastVertices[i + 2] << std::endl;
                for (size_t i = 0; i < chunk->m_terrainIndices.size(); i += 3)
                    objOut << "f " << (indexOffset + chunk->m_terrainIndices[i + 0])
                           << " "  << (indexOffset + chunk->m_terrainIndices[i + 1])
                           << " "  << (indexOffset + chunk->m_terrainIndices[i + 2]) << std::endl;
            }

            indexOffset += static_cast<int>(chunk->m_terrainVertices.size());

            if (chunk->m_liquidVertices.size() && chunk->m_liquidIndices.size())
            {
                objOut << "# ADT liquid" << std::endl;

                utility::Convert::VerticesToRecast(chunk->m_liquidVertices, recastVertices);
                for (size_t i = 0; i < recastVertices.size(); i += 3)
                    objOut << "v " << recastVertices[i + 0] << " " << recastVertices[i + 1] << " " << recastVertices[i + 2] << std::endl;
                for (size_t i = 0; i < chunk->m_liquidIndices.size(); i += 3)
                    objOut << "f " << (indexOffset + chunk->m_liquidIndices[i + 0])
                           << " "  << (indexOffset + chunk->m_liquidIndices[i + 1])
                           << " "  << (indexOffset + chunk->m_liquidIndices[i + 2]) << std::endl;
                
                indexOffset += static_cast<int>(chunk->m_liquidVertices.size());
            }
        }

    // wmos (and included doodads and liquid)
    for (auto const &wmoId : tile->WmoInstances)
    {
        auto const wmoInstance = m_map->GetWmoInstance(wmoId);

        if (!wmoInstance)
        {
            std::stringstream str;
            str << "Could not find required WMO ID = " << wmoId << " needed by ADT (" << adtX << ", " << adtY << ")\n";
            std::cout << str.str();
        }

        assert(!!wmoInstance);

        std::vector<utility::Vertex> vertices;
        std::vector<float> recastVertices;
        std::vector<int> indices;

        wmoInstance->BuildTriangles(vertices, indices);
        utility::Convert::VerticesToRecast(vertices, recastVertices);

        objOut << "# WMO terrain (" << wmoInstance->Model->FileName << ")" << std::endl;
        for (size_t i = 0; i < recastVertices.size(); i += 3)
            objOut << "v " << recastVertices[i + 0] << " " << recastVertices[i + 1] << " " << recastVertices[i + 2] << std::endl;
        for (size_t i = 0; i < indices.size(); i += 3)
            objOut << "f " << (indexOffset + indices[i + 0])
                   << " "  << (indexOffset + indices[i + 1])
                   << " "  << (indexOffset + indices[i + 2]) << std::endl;

        indexOffset += static_cast<int>(vertices.size());

        wmoInstance->BuildLiquidTriangles(vertices, indices);
        utility::Convert::VerticesToRecast(vertices, recastVertices);

        objOut << "# WMO liquid (" << wmoInstance->Model->FileName << ")" << std::endl;
        for (size_t i = 0; i < recastVertices.size(); i += 3)
            objOut << "v " << recastVertices[i + 0] << " " << recastVertices[i + 1] << " " << recastVertices[i + 2] << std::endl;
        for (size_t i = 0; i < indices.size(); i += 3)
            objOut << "f " << (indexOffset + indices[i + 0])
                   << " "  << (indexOffset + indices[i + 1])
                   << " "  << (indexOffset + indices[i + 2]) << std::endl;

        indexOffset += static_cast<int>(vertices.size());

        wmoInstance->BuildDoodadTriangles(vertices, indices);
        utility::Convert::VerticesToRecast(vertices, recastVertices);

        objOut << "# WMO doodads (" << wmoInstance->Model->FileName << ")" << std::endl;
        for (size_t i = 0; i < recastVertices.size(); i += 3)
            objOut << "v " << recastVertices[i + 0] << " " << recastVertices[i + 1] << " " << recastVertices[i + 2] << std::endl;
        for (size_t i = 0; i < indices.size(); i += 3)
            objOut << "f " << (indexOffset + indices[i + 0])
                   << " "  << (indexOffset + indices[i + 1])
                   << " "  << (indexOffset + indices[i + 2]) << std::endl;

        indexOffset += static_cast<int>(vertices.size());
    }

    // doodads
    for (auto const &doodadId : tile->DoodadInstances)
    {
        auto const doodadInstance = m_map->GetDoodadInstance(doodadId);

        assert(!!doodadInstance);

        std::vector<utility::Vertex> vertices;
        std::vector<float> recastVertices;
        std::vector<int> indices;

        doodadInstance->BuildTriangles(vertices, indices);
        utility::Convert::VerticesToRecast(vertices, recastVertices);

        objOut << "# Doodad (" << doodadInstance->Model->FileName << ")" << std::endl;
        for (size_t i = 0; i < recastVertices.size(); i += 3)
            objOut << "v " << recastVertices[i + 0] << " " << recastVertices[i + 1] << " " << recastVertices[i + 2] << std::endl;
        for (size_t i = 0; i < indices.size(); i += 3)
            objOut << "f " << (indexOffset + indices[i + 0])
                   << " "  << (indexOffset + indices[i + 1])
                   << " "  << (indexOffset + indices[i + 2]) << std::endl;

        indexOffset += static_cast<int>(vertices.size());
    }

    return true;
}

void MeshBuilder::SaveMap() const
{
    std::stringstream filename;
    filename << m_outputPath << "\\" << m_map->Name << ".map";

    std::ofstream out(filename.str(), std::ofstream::binary|std::ofstream::trunc);

    m_map->Serialize(out);
}