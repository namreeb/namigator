#include "MeshBuilder.hpp"

#include "BVHConstructor.hpp"
#include "Common.hpp"
#include "FileExist.hpp"
#include "RecastContext.hpp"
#include "parser/Adt/Adt.hpp"
#include "parser/Adt/AdtChunk.hpp"
#include "parser/DBC.hpp"
#include "recastnavigation/Detour/Include/DetourAlloc.h"
#include "recastnavigation/Detour/Include/DetourNavMeshBuilder.h"
#include "recastnavigation/Recast/Include/Recast.h"
#include "utility/AABBTree.hpp"
#include "utility/Exception.hpp"
#include "utility/MathHelper.hpp"
#include "utility/Vector.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define ZERO(x) memset(&x, 0, sizeof(x))

static_assert(sizeof(int) == sizeof(std::int32_t),
              "Recast requires 32 bit int type");
static_assert(sizeof(float) == 4, "float must be a 32 bit type");
static_assert(sizeof(unsigned char) == 1, "unsigned char must be size 1");

namespace
{
fs::path BuildAbsoluteFilename(const fs::path& outputPath,
                               const std::string& in)
{
    const fs::path path(in);
    auto const filename = path.filename().replace_extension("bvh").string();

    auto const extension = path.extension().string();

    std::string kind;
    if (extension[1] == 'm' || extension[1] == 'M')
        kind = "Doodad";
    else if (extension[1] == 'w' || extension[1] == 'W')
        kind = "WMO";
    else
        THROW(Result::UNRECOGNIZED_EXTENSION);

    return outputPath / "BVH" / (kind + "_" + filename);
}

// multiple chunks are often required even though a tile is guarunteed to be no
// bigger than a chunk. this is because recast requires geometry from
// neighboring tiles to produce accurate results.
void ComputeRequiredChunks(const parser::Map* map, int tileX, int tileY,
                           std::vector<std::pair<int, int>>& chunks)
{
    chunks.clear();

    constexpr float ChunksPerTile =
        MeshSettings::TileSize / MeshSettings::AdtChunkSize;

    // NOTE: these are global chunk coordinates, not ADT or tile
    auto const startX =
        (std::max)(0, static_cast<int>((tileX - 1) * ChunksPerTile));
    auto const startY =
        (std::max)(0, static_cast<int>((tileY - 1) * ChunksPerTile));
    auto const stopX = static_cast<int>((tileX + 1) * ChunksPerTile);
    auto const stopY = static_cast<int>((tileY + 1) * ChunksPerTile);

    for (auto y = startY; y <= stopY; ++y)
        for (auto x = startX; x <= stopX; ++x)
        {
            auto const adtX = x / MeshSettings::ChunksPerAdt;
            auto const adtY = y / MeshSettings::ChunksPerAdt;

            if (adtX < 0 || adtY < 0 || adtX >= MeshSettings::Adts ||
                adtY >= MeshSettings::Adts)
                continue;

            if (!map->HasAdt(adtX, adtY))
                continue;

            // put the chunk for the requested tile at the start of the results
            if (x == tileX * ChunksPerTile && y == tileY * ChunksPerTile)
                chunks.insert(chunks.begin(), {x, y});
            else
                chunks.push_back({x, y});
        }
}

bool TransformAndRasterize(rcContext& ctx, rcHeightfield& heightField,
                           float slope,
                           const std::vector<math::Vertex>& vertices,
                           const std::vector<int>& indices,
                           unsigned char areaFlags)
{
    if (!vertices.size() || !indices.size())
        return true;

    std::vector<float> rastVert;
    math::Convert::VerticesToRecast(vertices, rastVert);

    std::vector<unsigned char> areas(indices.size() / 3, areaFlags);

    // if these triangles are ADT, mark steep
    if (areaFlags & PolyFlags::Ground)
    {
        rcMarkWalkableTriangles(
            &ctx, slope, &rastVert[0], static_cast<int>(vertices.size()),
            &indices[0], static_cast<int>(indices.size() / 3), &areas[0]);

        // this will override the area for all walkable triangles, but what we
        // want is to flag the unwalkable ones.  so a little massaging of flags
        // is necessary here
        for (auto& a : areas)
        {
            if (a == RC_WALKABLE_AREA)
                a = areaFlags;
            else
                a = areaFlags | PolyFlags::Steep;
        }
    }
    // otherwise, if they are from doodads, clear steep
    else if (areaFlags & PolyFlags::Doodad)
        rcClearUnwalkableTriangles(
            &ctx, slope, &rastVert[0], static_cast<int>(vertices.size()),
            &indices[0], static_cast<int>(indices.size() / 3), &areas[0]);

    return rcRasterizeTriangles(
        &ctx, &rastVert[0], static_cast<int>(vertices.size()), &indices[0],
        &areas[0], static_cast<int>(indices.size() / 3), heightField, -1);
}

void FilterGroundBeneathLiquid(rcHeightfield& solid)
{
    for (int i = 0; i < solid.width * solid.height; ++i)
    {
        // as we go, we build a list of spans which will be removed in the event
        // we find liquid
        std::vector<rcSpan*> spans;

        for (rcSpan* s = solid.spans[i]; s; s = s->next)
        {
            // if we found a non-wmo liquid span, remove everything beneath it
            if (!!(s->area & PolyFlags::Liquid) && !(s->area & PolyFlags::Wmo))
            {
                for (auto ns : spans)
                    ns->area = RC_NULL_AREA;

                spans.clear();
            }
            // if we found a wmo liquid span, remove every wmo span beneath it
            else if (!!(s->area & (PolyFlags::Liquid | PolyFlags::Wmo)))
            {
                for (auto ns : spans)
                    if (!!(ns->area & PolyFlags::Wmo))
                        ns->area = RC_NULL_AREA;

                spans.clear();
            }
            else
                spans.push_back(s);
        }
    }
}

// NOTE: this does not set bmin/bmax
void InitializeRecastConfig(rcConfig& config)
{
    ZERO(config);

    config.cs = MeshSettings::CellSize;
    config.ch = MeshSettings::CellHeight;
    config.walkableSlopeAngle = MeshSettings::WalkableSlope;
    config.walkableClimb = MeshSettings::VoxelWalkableClimb;
    config.walkableHeight = MeshSettings::VoxelWalkableHeight;
    config.walkableRadius = MeshSettings::VoxelWalkableRadius;
    config.maxEdgeLen = config.walkableRadius * 4;
    config.maxSimplificationError = MeshSettings::MaxSimplificationError;
    config.minRegionArea = MeshSettings::MinRegionSize;
    config.mergeRegionArea = MeshSettings::MergeRegionSize;
    config.maxVertsPerPoly = MeshSettings::VerticesPerPolygon;
    config.tileSize = MeshSettings::TileVoxelSize;
    config.borderSize = config.walkableRadius + 3;
    config.width = config.tileSize + config.borderSize * 2;
    config.height = config.tileSize + config.borderSize * 2;
    config.detailSampleDist = MeshSettings::DetailSampleDistance;
    config.detailSampleMaxError = MeshSettings::DetailSampleMaxError;
}

void SerializeWMOAndDoodadIDs(const std::unordered_set<std::uint32_t>& wmos,
                              const std::unordered_set<std::uint32_t>& doodads,
                              utility::BinaryStream& out)
{
    utility::BinaryStream result(sizeof(std::uint32_t) *
                                 (2 + wmos.size() + doodads.size()));

    result << static_cast<std::uint32_t>(wmos.size());

    for (auto const& wmo : wmos)
        result << wmo;

    result << static_cast<std::uint32_t>(doodads.size());

    for (auto const& doodad : doodads)
        result << doodad;

    out = std::move(result);
}

void SerializeHeightField(const rcHeightfield& solid,
                          utility::BinaryStream& out)
{
    utility::BinaryStream result(sizeof(std::uint32_t) *
                                 (11 + 3 * (solid.width * solid.height)));

    result << static_cast<std::int32_t>(solid.width)
           << static_cast<std::int32_t>(solid.height);

    result.Write(&solid.bmin, sizeof(solid.bmin));
    result.Write(&solid.bmax, sizeof(solid.bmax));

    result << solid.cs << solid.ch;

    // TODO this might be storable in less space, since rcSpan is a bitfield
    // struct
    for (auto i = 0; i < solid.width * solid.height; ++i)
    {
        // place holder for the height of this column in the height field
        auto const columnSize = result.wpos();
        result << static_cast<std::uint32_t>(0);

        auto count = 0;
        for (const rcSpan* s = solid.spans[i]; !!s; s = s->next)
        {
            result << static_cast<std::uint32_t>(s->smin)
                   << static_cast<std::uint32_t>(s->smax)
                   << static_cast<std::uint32_t>(s->area);
            ++count;
        }

        auto const height =
            (result.wpos() - columnSize + sizeof(std::uint32_t)) /
            (3 * sizeof(std::uint32_t));

        result.Write(columnSize, static_cast<std::uint32_t>(height));
    }

    out = std::move(result);
}

void SerializeTileQuadHeight(const parser::AdtChunk* chunk, int tileX,
                             int tileY, utility::BinaryStream& out)
{
    // how many quads are on this tile
    auto constexpr width = 8 / MeshSettings::TilesPerChunk;

    utility::BinaryStream result(1 + 4 * 2 + width * width +
                                 sizeof(float) *
                                     MeshSettings::QuadValuesPerTile);

    // the '1' signifies that quad height data follows
    result << static_cast<std::uint8_t>(1);

    result << chunk->m_zoneId;
    result << chunk->m_areaId;

    auto const startX = (tileX % MeshSettings::TilesPerChunk) * width;
    auto const startY = (tileY % MeshSettings::TilesPerChunk) * width;

    auto constexpr maxIdx =
        sizeof(chunk->m_heights) / sizeof(chunk->m_heights[0]);

    // write holes
    for (auto y = startY; y < startY + width; ++y)
        for (auto x = startX; x < startX + width; ++x)
            result << static_cast<std::uint8_t>(chunk->m_holeMap[x][y] ? 1 : 0);

    // add the first row of values
    for (auto x = 0; x <= width; ++x)
    {
        auto const idx = 17 * startY + x + startX;
        assert(idx < maxIdx);

        result << chunk->m_heights[idx];
    }

    // middle values and the bottom row for each remaining quad
    for (auto y = 0; y < width; ++y)
    {
        // middle values
        for (auto x = 0; x < width; ++x)
        {
            auto const idx = 17 * (y + startY) + x + startX + 9;
            assert(idx < maxIdx);

            result << chunk->m_heights[idx];
        }

        // bottom values
        for (auto x = 0; x <= width; ++x)
        {
            auto const idx = 17 * (y + startY + 1) + x + startX;
            assert(idx < maxIdx);

            result << chunk->m_heights[idx];
        }
    }

    out = std::move(result);
}

using SmartHeightFieldPtr =
    std::unique_ptr<rcHeightfield, decltype(&rcFreeHeightField)>;
using SmartHeightFieldLayerSetPtr =
    std::unique_ptr<rcHeightfieldLayerSet,
                    decltype(&rcFreeHeightfieldLayerSet)>;
using SmartCompactHeightFieldPtr =
    std::unique_ptr<rcCompactHeightfield, decltype(&rcFreeCompactHeightfield)>;
using SmartContourSetPtr =
    std::unique_ptr<rcContourSet, decltype(&rcFreeContourSet)>;
using SmartPolyMeshPtr = std::unique_ptr<rcPolyMesh, decltype(&rcFreePolyMesh)>;
using SmartPolyMeshDetailPtr =
    std::unique_ptr<rcPolyMeshDetail, decltype(&rcFreePolyMeshDetail)>;

bool SerializeMeshTile(rcContext& ctx, const rcConfig& config, int tileX,
                       int tileY, rcHeightfield& solid,
                       utility::BinaryStream& out)
{
    // initialize compact height field
    SmartCompactHeightFieldPtr chf(rcAllocCompactHeightfield(),
                                   rcFreeCompactHeightfield);

    if (!rcBuildCompactHeightfield(&ctx, config.walkableHeight,
                                   (std::numeric_limits<int>::max)(), solid,
                                   *chf))
        return false;

    if (!rcBuildDistanceField(&ctx, *chf))
        return false;

    if (!rcBuildRegions(&ctx, *chf, config.borderSize, config.minRegionArea,
                        config.mergeRegionArea))
        return false;

    SmartContourSetPtr cset(rcAllocContourSet(), rcFreeContourSet);

    if (!rcBuildContours(&ctx, *chf, config.maxSimplificationError,
                         config.maxEdgeLen, *cset))
        return false;

    // it is possible that this tile has no navigable geometry.  in this case,
    // we 'succeed' by doing nothing further
    if (!cset->nconts)
        return true;

    SmartPolyMeshPtr polyMesh(rcAllocPolyMesh(), rcFreePolyMesh);

    if (!rcBuildPolyMesh(&ctx, *cset, config.maxVertsPerPoly, *polyMesh))
        return false;

    SmartPolyMeshDetailPtr polyMeshDetail(rcAllocPolyMeshDetail(),
                                          rcFreePolyMeshDetail);

    if (!rcBuildPolyMeshDetail(&ctx, *polyMesh, *chf, config.detailSampleDist,
                               config.detailSampleMaxError, *polyMeshDetail))
        return false;

    chf.reset();
    cset.reset();

    // too many vertices?
    if (polyMesh->nverts >= 0xFFFF)
    {
        std::stringstream str;
        str << "Too many mesh vertices produces for tile (" << tileX << ", "
            << tileY << ")" << std::endl;
        std::cerr << str.str();
        return false;
    }

    // TODO: build flags[] based on areas[], and replace areas[] with wow area
    // ids (?)
    for (int i = 0; i < polyMesh->npolys; ++i)
    {
        if (!polyMesh->areas[i])
            continue;

        polyMesh->flags[i] = static_cast<unsigned short>(polyMesh->areas[i]);
        polyMesh->areas[i] = 0;
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
    params.walkableHeight = MeshSettings::WalkableHeight;
    params.walkableRadius = MeshSettings::WalkableRadius;
    params.walkableClimb = MeshSettings::WalkableClimb;
    params.tileX = tileX;
    params.tileY = tileY;
    params.tileLayer = 0;
    memcpy(params.bmin, polyMesh->bmin, sizeof(polyMesh->bmin));
    memcpy(params.bmax, polyMesh->bmax, sizeof(polyMesh->bmax));
    params.cs = config.cs;
    params.ch = config.ch;
    params.buildBvTree = true;

    unsigned char* outData;
    int outDataSize;
    if (!dtCreateNavMeshData(&params, &outData, &outDataSize))
        return false;

    utility::BinaryStream result(outDataSize);
    result.Write(outData, outDataSize);

    out = std::move(result);

    dtFree(outData);

    return true;
}

bool IsHeightFieldEmpty(rcHeightfield const& solid)
{
    for (auto i = 0; i < solid.width * solid.height; ++i)
        if (!!solid.spans[i])
            return false;

    return true;
}
} // namespace

MeshBuilder::MeshBuilder(const std::filesystem::path& outputPath,
                         const std::string& mapName, int logLevel)
    : m_outputPath(outputPath), m_bvhConstructor(outputPath),
      m_chunkReferences(MeshSettings::ChunkCount * MeshSettings::ChunkCount),
      m_completedTiles(0), m_logLevel(logLevel)
{
    // this must follow the parser initialization
    m_map = std::make_unique<parser::Map>(mapName);

    if (auto const wmo = m_map->GetGlobalWmoInstance())
    {
        auto const tileWidth = static_cast<int>(
            ::ceilf((wmo->Bounds.MaxCorner.X - wmo->Bounds.MinCorner.X) /
                    MeshSettings::TileSize));
        auto const tileHeight = static_cast<int>(
            ::ceilf((wmo->Bounds.MaxCorner.Y - wmo->Bounds.MinCorner.Y) /
                    MeshSettings::TileSize));

        m_globalWMO = std::make_unique<meshfiles::GlobalWMO>();

        for (auto tileY = 0; tileY < tileHeight; ++tileY)
            for (auto tileX = 0; tileX < tileWidth; ++tileX)
                m_pendingTiles.push_back({tileX, tileY});

        wmo->BuildTriangles(m_globalWMOVertices, m_globalWMOIndices);
        wmo->BuildLiquidTriangles(m_globalWMOLiquidVertices,
                                  m_globalWMOLiquidIndices);
        wmo->BuildDoodadTriangles(m_globalWMODoodadVertices,
                                  m_globalWMODoodadIndices);

        std::vector<float> vertices;
        math::Convert::VerticesToRecast(m_globalWMOVertices, vertices);

        m_minX = m_maxX = vertices[0];
        m_minY = m_maxY = vertices[1];
        m_minZ = m_maxZ = vertices[2];

        for (size_t i = 3; i < vertices.size(); i += 3)
        {
            auto const x = vertices[i + 0];
            auto const y = vertices[i + 1];
            auto const z = vertices[i + 2];

            m_minX = (std::min)(m_minX, x);
            m_maxX = (std::max)(m_maxX, x);

            m_minY = (std::min)(m_minY, y);
            m_maxY = (std::max)(m_maxY, y);

            m_minZ = (std::min)(m_minZ, z);
            m_maxZ = (std::max)(m_maxZ, z);
        }
    }
    else
    {
        // this build order should ensure that the tiles for one ADT finish
        // before the next ADT begins (more or less)
        for (auto y = MeshSettings::Adts - 1; y >= 0; --y)
            for (auto x = MeshSettings::Adts - 1; x >= 0; --x)
            {
                if (!m_map->HasAdt(x, y))
                    continue;

                for (auto tileY = 0; tileY < MeshSettings::TilesPerADT; ++tileY)
                    for (auto tileX = 0; tileX < MeshSettings::TilesPerADT;
                         ++tileX)
                    {
                        auto const globalTileX =
                            x * MeshSettings::TilesPerADT + tileX;
                        auto const globalTileY =
                            y * MeshSettings::TilesPerADT + tileY;

                        std::vector<std::pair<int, int>> chunks;
                        ComputeRequiredChunks(m_map.get(), globalTileX,
                                              globalTileY, chunks);

                        for (auto const& chunk : chunks)
                            AddChunkReference(chunk.first, chunk.second);

                        m_pendingTiles.push_back({globalTileX, globalTileY});
                    }
            }
    }

    m_totalTiles = m_pendingTiles.size();

    files::create_nav_output_directory_for_map(m_outputPath, mapName);
}

MeshBuilder::MeshBuilder(const std::filesystem::path& outputPath,
                         const std::string& mapName, int logLevel, int adtX,
                         int adtY)
    : m_outputPath(outputPath), m_bvhConstructor(outputPath),
      m_chunkReferences(MeshSettings::ChunkCount * MeshSettings::ChunkCount),
      m_completedTiles(0), m_logLevel(logLevel)
{
    // this must follow the parser initialization
    m_map = std::make_unique<parser::Map>(mapName);

    for (auto y = adtY * MeshSettings::TilesPerADT;
         y < (adtY + 1) * MeshSettings::TilesPerADT; ++y)
        for (auto x = adtX * MeshSettings::TilesPerADT;
             x < (adtX + 1) * MeshSettings::TilesPerADT; ++x)
            m_pendingTiles.push_back({x, y});

    for (auto const& tile : m_pendingTiles)
    {
        std::vector<std::pair<int, int>> chunks;
        ComputeRequiredChunks(m_map.get(), tile.first, tile.second, chunks);

        for (auto chunk : chunks)
            AddChunkReference(chunk.first, chunk.second);
    }

    m_totalTiles = m_pendingTiles.size();


    files::create_nav_output_directory_for_map(m_outputPath, mapName);
}

void MeshBuilder::LoadGameObjects(const std::string& path)
{
    std::cout << "Reading game object..." << std::endl;

    std::ifstream in(path);

    if (in.fail())
        THROW(Result::FAILED_TO_OPEN_GAMEOBJECT_FILE).ErrorCode();

    m_gameObjectInstances.clear();

    std::string line;
    std::vector<std::string> cells;
    while (std::getline(in, line))
    {
        std::istringstream str(line);
        std::string field;

        while (std::getline(str, field, ','))
            cells.emplace_back(field);
    }

    in.close();

    if (!!(cells.size() % 10))
        THROW(Result::BAD_FORMAT_OF_GAMEOBJECT_FILE);

    m_gameObjectInstances.reserve(cells.size() / 10);

    for (auto i = 0u; i < cells.size(); i += 10)
    {
        GameObjectInstance instance {
            std::stoull(cells[i]),                                // guid
            static_cast<std::uint32_t>(std::stoul(cells[i + 1])), // display id
            static_cast<std::uint32_t>(std::stoul(cells[i + 2])), // map
            std::stof(cells[i + 3]),                              // position x
            std::stof(cells[i + 4]),                              // position y
            std::stof(cells[i + 5]),                              // position z
            std::stof(cells[i + 6]),                              // quaternion
            std::stof(cells[i + 7]),
            std::stof(cells[i + 8]),
            std::stof(cells[i + 9]),
        };

        // only concern ourselves with gameobjects on the current map
        if (instance.map == m_map->Id)
            m_gameObjectInstances.emplace_back(instance);
    }

    std::cout << "Loaded " << m_gameObjectInstances.size()
              << " game object instances.  Loading model information..."
              << std::endl;

    auto const displayInfo =
        parser::DBC("DBFilesClient\\GameObjectDisplayInfo.dbc");

    std::unordered_map<std::uint32_t, math::AABBTree> models;

    for (auto i = 0u; i < displayInfo.RecordCount(); ++i)
    {
        auto const id = displayInfo.GetField(i, 0);
        auto const modelPath = displayInfo.GetStringField(i, 1);

        // not sure why this happens.  might we be missing some needed
        // information?
        if (modelPath.length() == 0)
            continue;

        auto const extension = fs::path(modelPath).extension().string();

        // doodad
        if (extension[1] == 'm' || extension[1] == 'M')
        {
            auto const doodad = m_map->GetDoodad(modelPath);

            int asdf = 1234;
        }
        // wmo
        else if (extension[1] == 'w' || extension[1] == 'W')
        {
            auto const wmo = m_map->GetWmo(modelPath);

            int asdf = 1234;
        }
        else
            THROW(Result::UNRECOGNIZED_MODEL_EXTENSION);

        auto const modelFullPath =
            BuildAbsoluteFilename(m_outputPath, modelPath);

        // if the file fails to open, continue to the next one silently.  this
        // happens when the model has no collision geometry
        if (std::ifstream(modelFullPath, std::ifstream::binary).fail())
            continue;

        utility::BinaryStream modelFile(modelFullPath);

        math::AABBTree model;

        if (!model.Deserialize(modelFile))
        {
            std::cerr << "Failed to deserialize model " << modelPath
                      << std::endl;
            continue;
        }

        models[id] = std::move(model);
    }

    std::cout << "Done.  Loaded " << models.size()
              << " models.  Instantiating game objects..." << std::endl;

    for (auto const& go : m_gameObjectInstances)
    {
        if (models.find(go.displayId) == models.end())
            THROW(Result::GAME_OBJECT_REFERENCES_NON_EXISTENT_MODEL_ID);
    }
}

bool MeshBuilder::GetNextTile(int& tileX, int& tileY)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    if (m_pendingTiles.empty())
        return false;

    auto const back = m_pendingTiles.back();
    m_pendingTiles.pop_back();

    tileX = back.first;
    tileY = back.second;

    return true;
}

bool MeshBuilder::IsGlobalWMO() const
{
    return !!m_map->GetGlobalWmoInstance();
}

void MeshBuilder::AddChunkReference(int chunkX, int chunkY)
{
    assert(chunkX >= 0 && chunkY >= 0 && chunkX < MeshSettings::ChunkCount &&
           chunkY < MeshSettings::ChunkCount);
    ++m_chunkReferences[chunkY * MeshSettings::ChunkCount + chunkX];
}

void MeshBuilder::RemoveChunkReference(int chunkX, int chunkY)
{
    auto const adtX = chunkX / MeshSettings::ChunksPerAdt;
    auto const adtY = chunkY / MeshSettings::ChunksPerAdt;

    std::lock_guard<std::mutex> guard(m_mutex);
    --m_chunkReferences[chunkY * MeshSettings::ChunkCount + chunkX];

    // check to see if all chunks on this ADT are without references.  if so,
    // unload the ADT
    bool unload = true;
    for (int x = 0; x < MeshSettings::ChunksPerAdt; ++x)
        for (int y = 0; y < MeshSettings::ChunksPerAdt; ++y)
        {
            auto const globalChunkX = adtX * MeshSettings::ChunksPerAdt + x;
            auto const globalChunkY = adtY * MeshSettings::ChunksPerAdt + y;

            if (m_chunkReferences[globalChunkY * MeshSettings::ChunkCount +
                                  globalChunkX] > 0)
            {
                unload = false;
                break;
            }
        }

    if (unload)
    {
#ifdef _DEBUG
        std::stringstream str;
        str << "No threads need ADT (" << std::setfill(' ') << std::setw(2)
            << adtX << ", " << std::setfill(' ') << std::setw(2) << adtY
            << ").  Unloading.\n";
        std::cout << str.str();
#endif

        m_map->UnloadAdt(adtX, adtY);
    }
}

void MeshBuilder::SerializeWmo(const parser::Wmo& wmo)
{
    if (m_bvhWmos.find(wmo.MpqPath) != m_bvhWmos.end())
        return;

    meshfiles::SerializeWmo(wmo, m_bvhConstructor);

    // the above serialization routine will also serialize all doodads in all
    // doodad sets for this wmo, therefore we should mark them as serialized

    for (auto const& doodadSet : wmo.DoodadSets)
        for (auto const& wmoDoodad : doodadSet)
            m_bvhDoodads.insert(wmoDoodad->Parent->MpqPath);

    m_bvhWmos.insert(wmo.MpqPath);
}

void MeshBuilder::SerializeDoodad(const parser::Doodad& doodad)
{
    if (m_bvhDoodads.find(doodad.MpqPath) != m_bvhDoodads.end())
        return;

    meshfiles::SerializeDoodad(doodad,
                               m_bvhConstructor.AddFile(doodad.MpqPath));

    m_bvhDoodads.insert(doodad.MpqPath);
}

bool MeshBuilder::BuildAndSerializeWMOTile(int tileX, int tileY)
{
    auto const wmoInstance = m_map->GetGlobalWmoInstance();

    assert(!!wmoInstance);

    SerializeWmo(*wmoInstance->Model);

    rcConfig config;
    InitializeRecastConfig(config);

    // tile coordinates are calculated offset from the northwest corner
    auto const westStart =
        wmoInstance->Bounds.MaxCorner.Y -
        (tileX * MeshSettings::TileSize); // wow coordinate system
    auto const northStart =
        wmoInstance->Bounds.MaxCorner.X -
        (tileY * MeshSettings::TileSize); // wow coordinate system

    config.bmin[0] = -westStart;
    config.bmin[1] = wmoInstance->Bounds.MinCorner.Z;
    config.bmin[2] = -northStart;

    config.bmax[0] = config.bmin[0] + MeshSettings::TileSize;
    config.bmax[1] = wmoInstance->Bounds.MaxCorner.Z;
    config.bmax[2] = config.bmin[2] + MeshSettings::TileSize;

    // bounding box of tile in wow coordinates for culling wmos and doodads
    const math::BoundingBox tileBounds(
        {config.bmax[2], config.bmax[0], config.bmin[1]},
        {config.bmin[2], config.bmin[0], config.bmax[1]});

    // erode mesh tile boundaries to force recast to examine obstacles on or
    // near the tile boundary
    config.bmin[0] -= config.borderSize * config.cs;
    config.bmin[2] -= config.borderSize * config.cs;
    config.bmax[0] += config.borderSize * config.cs;
    config.bmax[2] += config.borderSize * config.cs;

    RecastContext ctx(m_logLevel);

    SmartHeightFieldPtr solid(rcAllocHeightfield(), rcFreeHeightField);

    if (!rcCreateHeightfield(&ctx, *solid, config.width, config.height,
                             config.bmin, config.bmax, config.cs, config.ch))
        return false;

    // wmo terrain
    if (!TransformAndRasterize(ctx, *solid, config.walkableSlopeAngle,
                               m_globalWMOVertices, m_globalWMOIndices,
                               PolyFlags::Wmo))
        return false;

    // wmo liquid
    if (!TransformAndRasterize(
            ctx, *solid, config.walkableSlopeAngle, m_globalWMOLiquidVertices,
            m_globalWMOLiquidIndices, PolyFlags::Wmo | PolyFlags::Liquid))
        return false;

    // wmo doodads
    if (!TransformAndRasterize(ctx, *solid, config.walkableSlopeAngle,
                               m_globalWMODoodadVertices,
                               m_globalWMODoodadIndices, PolyFlags::Doodad))
        return false;

    auto const solidEmpty = IsHeightFieldEmpty(*solid);

    if (!solidEmpty)
    {
        FilterGroundBeneathLiquid(*solid);

        // note that no area id preservation is necessary here because we have
        // no ADT terrain
        rcFilterLedgeSpans(&ctx, config.walkableHeight, config.walkableClimb,
                           *solid);
        rcFilterWalkableLowHeightSpans(&ctx, config.walkableHeight, *solid);
        rcFilterLowHangingWalkableObstacles(&ctx, config.walkableClimb, *solid);
    }

    // serialize heightfield for this tile
    utility::BinaryStream heightFieldData(
        sizeof(std::uint32_t) * (10 + 3 * (solid->width * solid->height)));

    if (!solidEmpty)
        SerializeHeightField(*solid, heightFieldData);

    // serialize final navmesh tile
    utility::BinaryStream meshData(0);
    if (!solidEmpty)
    {
        auto const result =
            SerializeMeshTile(ctx, config, tileX, tileY, *solid, meshData);
        assert(result);
    }

    std::lock_guard<std::mutex> guard(m_mutex);

    if (!solidEmpty)
        m_globalWMO->AddTile(tileX, tileY, heightFieldData, meshData);

    if (++m_completedTiles == m_totalTiles)
    {
        m_globalWMO->Serialize(m_outputPath / "Nav" / m_map->Name / "Map.nav");

#ifdef _DEBUG
        std::stringstream log;
        log << "Finished " << m_map->Name << ".";
        std::cout << log.str() << std::endl;
#endif
    }

    return true;
}

bool MeshBuilder::BuildAndSerializeMapTile(int tileX, int tileY)
{
    float minZ = (std::numeric_limits<float>::max)(),
          maxZ = std::numeric_limits<float>::lowest();

    // regardless of tile size, we only need the surrounding ADT chunks
    std::vector<const parser::AdtChunk*> chunks;
    std::vector<std::pair<int, int>> chunkPositions;

    ComputeRequiredChunks(m_map.get(), tileX, tileY, chunkPositions);

    chunks.reserve(chunkPositions.size());
    for (auto const& chunkPosition : chunkPositions)
    {
        auto const adtX = chunkPosition.first / MeshSettings::ChunksPerAdt;
        auto const adtY = chunkPosition.second / MeshSettings::ChunksPerAdt;
        auto const chunkX = chunkPosition.first % MeshSettings::ChunksPerAdt;
        auto const chunkY = chunkPosition.second % MeshSettings::ChunksPerAdt;

        assert(adtX >= 0 && adtY >= 0 && adtX < MeshSettings::Adts &&
               adtY < MeshSettings::Adts);

        auto const adt = m_map->GetAdt(adtX, adtY);
        auto const chunk = adt->GetChunk(chunkX, chunkY);

        minZ = (std::min)(minZ, chunk->m_minZ);
        maxZ = (std::max)(maxZ, chunk->m_maxZ);

        chunks.push_back(chunk);
    }

    // because ComputeRequiredChunks places the chunk that this tile falls on at
    // the start of the collection, we know that the first element in the
    // 'chunks' collection is also the chunk upon which this tile falls.
    auto const tileChunk = chunks[0];

    rcConfig config;
    InitializeRecastConfig(config);

    config.bmin[0] =
        tileX * MeshSettings::TileSize - 32.f * MeshSettings::AdtSize;
    config.bmin[1] = minZ;
    config.bmin[2] =
        tileY * MeshSettings::TileSize - 32.f * MeshSettings::AdtSize;

    config.bmax[0] =
        (tileX + 1) * MeshSettings::TileSize - 32.f * MeshSettings::AdtSize;
    config.bmax[1] = maxZ;
    config.bmax[2] =
        (tileY + 1) * MeshSettings::TileSize - 32.f * MeshSettings::AdtSize;

    // bounding box of tile in wow coordinates for culling wmos and doodads
    const math::BoundingBox tileBounds(
        {-config.bmax[2], -config.bmax[0], config.bmin[1]},
        {-config.bmin[2], -config.bmin[0], config.bmax[1]});

    // erode mesh tile boundaries to force recast to examine obstacles on or
    // near the tile boundary
    config.bmin[0] -= config.borderSize * config.cs;
    config.bmin[2] -= config.borderSize * config.cs;
    config.bmax[0] += config.borderSize * config.cs;
    config.bmax[2] += config.borderSize * config.cs;

    RecastContext ctx(m_logLevel);

    SmartHeightFieldPtr solid(rcAllocHeightfield(), rcFreeHeightField);

    if (!rcCreateHeightfield(&ctx, *solid, config.width, config.height,
                             config.bmin, config.bmax, config.cs, config.ch))
        return false;

    std::unordered_set<std::uint32_t> rasterizedWmos;
    std::unordered_set<std::uint32_t> rasterizedDoodads;

    // incrementally rasterize mesh geometry into the height field, setting poly
    // flags as appropriate
    for (auto const& chunk : chunks)
    {
        // adt terrain
        if (!TransformAndRasterize(ctx, *solid, config.walkableSlopeAngle,
                                   chunk->m_terrainVertices,
                                   chunk->m_terrainIndices, PolyFlags::Ground))
            return false;

        // liquid
        if (!TransformAndRasterize(ctx, *solid, config.walkableSlopeAngle,
                                   chunk->m_liquidVertices,
                                   chunk->m_liquidIndices, PolyFlags::Liquid))
            return false;

        // wmos (and included doodads and liquid)
        for (auto const& wmoId : chunk->m_wmoInstances)
        {
            if (rasterizedWmos.find(wmoId) != rasterizedWmos.end())
                continue;

            auto const wmoInstance = m_map->GetWmoInstance(wmoId);

            if (!wmoInstance)
            {
                std::stringstream str;
                str << "Could not find required WMO ID = " << wmoId
                    << " needed by tile (" << tileX << ", " << tileY << ")";

                THROW_MSG(str.str(), Result::COULD_NOT_FIND_WMO);
            }

            assert(!!wmoInstance);

            // FIXME: it is possible that the added geometry of the doodads will
            // expand the bounding box of this wmo, thereby placing it on this
            // tile
            if (!tileBounds.intersect2d(wmoInstance->Bounds))
                continue;

            std::vector<math::Vertex> vertices;
            std::vector<int> indices;

            wmoInstance->BuildTriangles(vertices, indices);
            if (!TransformAndRasterize(ctx, *solid, config.walkableSlopeAngle,
                                       vertices, indices, PolyFlags::Wmo))
                return false;

            wmoInstance->BuildLiquidTriangles(vertices, indices);
            if (!TransformAndRasterize(ctx, *solid, config.walkableSlopeAngle,
                                       vertices, indices,
                                       PolyFlags::Wmo | PolyFlags::Liquid))
                return false;

            wmoInstance->BuildDoodadTriangles(vertices, indices);
            if (!TransformAndRasterize(ctx, *solid, config.walkableSlopeAngle,
                                       vertices, indices, PolyFlags::Doodad))
                return false;

            rasterizedWmos.insert(wmoId);
        }

        // doodads
        for (auto const& doodadId : chunk->m_doodadInstances)
        {
            if (rasterizedDoodads.find(doodadId) != rasterizedDoodads.end())
                continue;

            auto const doodadInstance = m_map->GetDoodadInstance(doodadId);

            assert(!!doodadInstance);

            if (!tileBounds.intersect2d(doodadInstance->Bounds))
                continue;

            std::vector<math::Vertex> vertices;
            std::vector<int> indices;

            doodadInstance->BuildTriangles(vertices, indices);
            if (!TransformAndRasterize(ctx, *solid, config.walkableSlopeAngle,
                                       vertices, indices, PolyFlags::Doodad))
                return false;

            rasterizedDoodads.insert(doodadId);
        }
    }

    FilterGroundBeneathLiquid(*solid);

    // we don't want to filter ledge spans from ADT terrain.  this will restore
    // the area for these spans, which we are using for flags
    {
        std::vector<std::pair<rcSpan*, unsigned int>> groundSpanAreas;

        groundSpanAreas.reserve(solid->width * solid->height);

        for (auto i = 0; i < solid->width * solid->height; ++i)
            for (rcSpan* s = solid->spans[i]; s; s = s->next)
                if (s->area & PolyFlags::Ground)
                    groundSpanAreas.push_back(std::pair<rcSpan*, unsigned int>(
                        s, static_cast<unsigned int>(s->area)));

        rcFilterLedgeSpans(&ctx, config.walkableHeight, config.walkableClimb,
                           *solid);

        for (auto p : groundSpanAreas)
            p.first->area = p.second;
    }

    rcFilterWalkableLowHeightSpans(&ctx, config.walkableHeight, *solid);
    rcFilterLowHangingWalkableObstacles(&ctx, config.walkableClimb, *solid);

    {
        std::lock_guard<std::mutex> guard(m_mutex);

        // Write the BVH for every new WMO
        for (auto const& wmoId : rasterizedWmos)
            SerializeWmo(*m_map->GetWmoInstance(wmoId)->Model);

        // Write the BVH for every new doodad
        for (auto const& doodadId : rasterizedDoodads)
            SerializeDoodad(*m_map->GetDoodadInstance(doodadId)->Model);
    }

    // serialize WMO and doodad IDs for this tile
    utility::BinaryStream wmosAndDoodads;
    SerializeWMOAndDoodadIDs(rasterizedWmos, rasterizedDoodads, wmosAndDoodads);

    // serialize heightfield for this tile
    utility::BinaryStream heightFieldData;
    SerializeHeightField(*solid, heightFieldData);

    // serialize ADT vertex height
    utility::BinaryStream quadHeightData;
    SerializeTileQuadHeight(tileChunk, tileX, tileY, quadHeightData);

    // serialize final navmesh tile
    utility::BinaryStream meshData;
    auto const result =
        SerializeMeshTile(ctx, config, tileX, tileY, *solid, meshData);

    {
        std::lock_guard<std::mutex> guard(m_mutex);

        auto const adtX = tileX / MeshSettings::TilesPerADT;
        auto const adtY = tileY / MeshSettings::TilesPerADT;
        auto const localTileX = tileX % MeshSettings::TilesPerADT;
        auto const localTileY = tileY % MeshSettings::TilesPerADT;

        auto adt = GetInProgressADT(adtX, adtY);

        adt->AddTile(localTileX, localTileY, wmosAndDoodads, quadHeightData,
                     heightFieldData, meshData);

        if (adt->IsComplete())
        {
            std::stringstream str;
            str << std::setw(2) << std::setfill('0') << adtX << "_"
                << std::setw(2) << std::setfill('0') << adtY << ".nav";

            adt->Serialize(m_outputPath / "Nav" / m_map->Name / str.str());

#ifdef _DEBUG
            std::stringstream log;
            log << "Finished " << m_map->Name << " ADT (" << adtX << ", "
                << adtY << ")";
            std::cout << log.str() << std::endl;
#endif

            RemoveADT(adt);
        }
    }

    ++m_completedTiles;
    for (auto const& chunk : chunkPositions)
        RemoveChunkReference(chunk.first, chunk.second);

    return result;
}

void MeshBuilder::SaveMap() const
{
    utility::BinaryStream out;
    m_map->Serialize(out);

    std::ofstream of(m_outputPath / (m_map->Name + ".map"),
                     std::ofstream::binary | std::ofstream::trunc);
    of << out;
}

float MeshBuilder::PercentComplete() const
{
    return 100.f * (float(m_completedTiles) / float(m_totalTiles));
}

meshfiles::ADT* MeshBuilder::GetInProgressADT(int x, int y)
{
    if (!m_adtsInProgress[{x, y}])
        m_adtsInProgress[{x, y}] = std::make_unique<meshfiles::ADT>(x, y);

    return m_adtsInProgress[{x, y}].get();
}

void MeshBuilder::RemoveADT(const meshfiles::ADT* adt)
{
    for (auto const& a : m_adtsInProgress)
        if (a.second.get() == adt)
        {
            m_adtsInProgress.erase(a.first);
            return;
        }
}

namespace meshfiles
{
void File::AddTile(int x, int y, utility::BinaryStream& heightfield,
                   const utility::BinaryStream& mesh)
{
    m_tiles[{x, y}] = std::move(heightfield);
    m_tiles[{x, y}] << static_cast<std::uint32_t>(mesh.wpos());
    m_tiles[{x, y}].Append(mesh);
}

void ADT::AddTile(int x, int y, utility::BinaryStream& wmosAndDoodads,
                  utility::BinaryStream& quadHeights,
                  utility::BinaryStream& heightField,
                  utility::BinaryStream& mesh)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    File::AddTile(x, y, heightField, mesh);

    m_wmosAndDoodadIds[{x, y}] = std::move(wmosAndDoodads);
    m_quadHeights[{x, y}] = std::move(quadHeights);
}

void ADT::Serialize(const fs::path& filename) const
{
    size_t bufferSize = 6 * sizeof(std::uint32_t);

    // first compute total size, just to reduce reallocations
    for (auto const& tile : m_tiles)
    {
        bufferSize += 2 * sizeof(std::uint32_t);

        // in this case, the wmo and doodad counts are included in the buffer
        if (m_wmosAndDoodadIds.find(tile.first) != m_wmosAndDoodadIds.end())
            bufferSize += m_wmosAndDoodadIds.at(tile.first).wpos();
        // if the buffer is empty, two zeros are written
        else
            bufferSize += 2 * sizeof(std::uint32_t);

        // quad height data
        bufferSize += m_quadHeights.at(tile.first).wpos();

        // height field and mesh buffer
        bufferSize += tile.second.wpos();
    }

    utility::BinaryStream outBuffer(bufferSize);

    // header
    outBuffer << MeshSettings::FileSignature << MeshSettings::FileVersion
              << MeshSettings::FileADT;

    // ADT x and y
    outBuffer << static_cast<std::uint32_t>(m_x)
              << static_cast<std::uint32_t>(m_y);

    // tile count
    outBuffer << static_cast<std::uint32_t>(m_tiles.size());

    for (auto const& tile : m_tiles)
    {
        // we want to store the global tile x and y, rather than the x, y
        // relative to this ADT
        auto const x = static_cast<std::uint32_t>(tile.first.first) +
                       m_x * MeshSettings::TilesPerADT;
        auto const y = static_cast<std::uint32_t>(tile.first.second) +
                       m_y * MeshSettings::TilesPerADT;

        outBuffer << x << y;

        // append wmo and doodad id buffer (which already contains size
        // information), or an empty one if there is none
        if (m_wmosAndDoodadIds.find(tile.first) == m_wmosAndDoodadIds.end())
            outBuffer << static_cast<std::uint32_t>(0)
                      << static_cast<std::uint32_t>(0);
        else
            outBuffer.Append(m_wmosAndDoodadIds.at(tile.first));

        // append adt quad height data
        outBuffer.Append(m_quadHeights.at(tile.first));

        // height field and finalized tile buffer
        outBuffer.Append(tile.second);
    }

    // just to make sure our calculation still works.  if it doesnt, we could
    // see copious reallocations in the above code!
    assert(outBuffer.wpos() == bufferSize);

    outBuffer.Compress();

    std::ofstream out(filename, std::ofstream::binary | std::ofstream::trunc);

    if (out.fail())
        THROW(Result::ADT_SERIALIZATION_FAILED_TO_OPEN_OUTPUT_FILE);

    out << outBuffer;
}

void GlobalWMO::AddTile(int x, int y, utility::BinaryStream& heightField,
                        utility::BinaryStream& mesh)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    File::AddTile(x, y, heightField, mesh);
}

void GlobalWMO::Serialize(const fs::path& filename) const
{
    size_t bufferSize = 6 * sizeof(std::uint32_t);

    // first compute total size, just to reduce reallocations
    for (auto const& tile : m_tiles)
        bufferSize += 4 * sizeof(std::uint32_t) + tile.second.wpos() +
                      sizeof(std::uint8_t);

    utility::BinaryStream outBuffer(bufferSize);

    // header
    outBuffer << MeshSettings::FileSignature << MeshSettings::FileVersion
              << MeshSettings::FileWMO;

    // ADT x and y
    outBuffer << MeshSettings::WMOcoordinate << MeshSettings::WMOcoordinate;

    // tile count
    outBuffer << static_cast<std::uint32_t>(m_tiles.size());

    for (auto const& tile : m_tiles)
    {
        // tile x and y
        outBuffer << tile.first.first << tile.first.second;

        // no per-tile doodad ids for global WMOs
        outBuffer << static_cast<std::uint32_t>(0)
                  << static_cast<std::uint32_t>(0);

        // the '0' indicates that there is no quad height data for this tile
        outBuffer << static_cast<std::uint8_t>(0);

        // height field and finalized tile buffer
        outBuffer.Append(tile.second);
    }

    // temporary just to make sure our calculation still works.  if it doesnt,
    // we could see copious reallocations in the above code!
    assert(outBuffer.wpos() == bufferSize);

    outBuffer.Compress();

    std::ofstream out(filename, std::ofstream::binary | std::ofstream::trunc);

    if (out.fail())
        THROW(Result::WMO_SERIALIZATION_FAILED_TO_OPEN_OUTPUT_FILE).ErrorCode();

    out << outBuffer;
}

void SerializeWmo(const parser::Wmo& wmo, BVHConstructor& constructor)
{
    math::AABBTree aabbTree(wmo.Vertices, wmo.Indices);

    utility::BinaryStream o;
    aabbTree.Serialize(o);

    o << static_cast<std::uint32_t>(wmo.RootId)
      << static_cast<std::uint32_t>(wmo.NameSetToAreaAndZone.size());

    for (auto const& i : wmo.NameSetToAreaAndZone)
        o << i;

    o << static_cast<std::uint32_t>(wmo.DoodadSets.size());

    for (auto const& doodadSet : wmo.DoodadSets)
    {
        o << static_cast<std::uint32_t>(doodadSet.size());

        for (auto const& wmoDoodad : doodadSet)
        {
            auto const doodad = wmoDoodad->Parent;

            o << wmoDoodad->TransformMatrix;
            o << wmoDoodad->Bounds;
            o.WriteString(doodad->MpqPath, MeshSettings::MaxMPQPathLength);

            // TODO: this probably causes the file to be written repeatedly
            // also serialize this doodad
            SerializeDoodad(*wmoDoodad->Parent.get(),
                            constructor.AddFile(doodad->MpqPath));
        }
    }

    auto const path = constructor.AddFile(wmo.MpqPath);

    std::ofstream of(path, std::ofstream::binary | std::ofstream::trunc);
    of << o;
}

void SerializeDoodad(const parser::Doodad& doodad, const fs::path& path)
{
    math::AABBTree doodadTree(doodad.Vertices, doodad.Indices);

    utility::BinaryStream doodadOut;
    doodadTree.Serialize(doodadOut);

    std::ofstream of(path, std::ofstream::binary | std::ofstream::trunc);
    of << doodadOut;
}
} // namespace meshfiles