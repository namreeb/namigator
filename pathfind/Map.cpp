#include "Map.hpp"
#include "Tile.hpp"

#include "utility/BinaryStream.hpp"
#include "utility/Exception.hpp"
#include "utility/MathHelper.hpp"
#include "utility/Ray.hpp"

#include "recastnavigation/Detour/Include/DetourCommon.h"
#include "recastnavigation/Detour/Include/DetourNavMesh.h"
#include "recastnavigation/Detour/Include/DetourNavMeshQuery.h"

#include "Common.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <limits>
#include <list>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

static_assert(sizeof(char) == 1, "char must be one byte");

static constexpr unsigned int GlobalWmoId = 0xFFFFFFFF;

#pragma pack(push, 1)
struct WmoFileInstance
{
    std::uint32_t m_id;
    std::uint16_t m_doodadSet;
    std::uint16_t m_nameSet;
    float m_transformMatrix[16];
    math::BoundingBox m_bounds;
    char m_fileName[MeshSettings::MaxMPQPathLength];
};

struct DoodadFileInstance
{
    std::uint32_t m_id;
    float m_transformMatrix[16];
    math::BoundingBox m_bounds;
    char m_fileName[MeshSettings::MaxMPQPathLength];
};

struct NavFileHeader
{
    std::uint32_t sig;
    std::uint32_t ver;
    std::uint32_t kind;
    std::uint32_t x;
    std::uint32_t y;
    std::uint32_t tileCount;

    void Verify(bool globalWmo) const
    {
        if (sig != MeshSettings::FileSignature)
            THROW("Incorrect file signature");

        if (ver != MeshSettings::FileVersion)
            THROW("Incorrect file version");

        if (globalWmo)
        {
            if (kind != MeshSettings::FileWMO)
                THROW("Not WMO nav file");

            if (x != MeshSettings::WMOcoordinate || y != MeshSettings::WMOcoordinate)
                THROW("Not WMO tile coordinates");
        }

        if (!globalWmo && kind != MeshSettings::FileADT)
            THROW("Not ADT nav file");
    }
};
#pragma pack(pop)

namespace pathfind
{
Map::Map(const std::string& dataPath, const std::string& mapName) : m_dataPath(dataPath), m_bvhLoader(dataPath), m_mapName(mapName)
{
    utility::BinaryStream in(m_dataPath / (mapName + ".map"));

    std::uint32_t magic;
    in >> magic;

    if (magic != MeshSettings::FileMap)
        THROW("Invalid map file");

    ::memset(m_loadedADT, 0, sizeof(m_loadedADT));

    std::uint8_t hasTerrain;
    in >> hasTerrain;

    assert(hasTerrain == 0 || hasTerrain == 1);

    if (hasTerrain)
    {
        std::uint8_t has_adt[MeshSettings::Adts * MeshSettings::Adts / 8];

        in.ReadBytes(has_adt, sizeof(has_adt));

        for (auto y = 0; y < MeshSettings::Adts; ++y)
            for (auto x = 0; x < MeshSettings::Adts; ++x)
            {
                auto const offset = y * MeshSettings::Adts + x;
                auto const byte_offset = offset / 8;
                auto const bit_offset = offset % 8;

                m_hasADT[x][y] = (has_adt[byte_offset] & (1 << bit_offset)) != 0;
            }

        dtNavMeshParams params;

        constexpr float mapOrigin = -32.f * MeshSettings::AdtSize;
        constexpr int maxTiles = MeshSettings::TileCount * MeshSettings::TileCount;

        params.orig[0] = mapOrigin;
        params.orig[1] = 0.f;
        params.orig[2] = mapOrigin;
        params.tileHeight = params.tileWidth = MeshSettings::TileSize;
        params.maxTiles = maxTiles;
        params.maxPolys = 1 << DT_POLY_BITS;

        auto const result = m_navMesh.init(&params);
        assert(result == DT_SUCCESS);

        std::uint32_t wmoInstanceCount;
        in >> wmoInstanceCount;

        if (wmoInstanceCount)
        {
            std::vector<WmoFileInstance> wmoInstances(wmoInstanceCount);
            in.ReadBytes(&wmoInstances[0], wmoInstanceCount * sizeof(WmoFileInstance));

            for (auto const& wmo : wmoInstances)
            {
                WmoInstance ins;

                ins.m_doodadSet = static_cast<unsigned int>(wmo.m_doodadSet);
                ins.m_nameSet = static_cast<unsigned int>(wmo.m_nameSet);
                ins.m_transformMatrix =
                    math::Matrix::CreateFromArray(wmo.m_transformMatrix, sizeof(wmo.m_transformMatrix) / sizeof(wmo.m_transformMatrix[0]));
                ins.m_inverseTransformMatrix = ins.m_transformMatrix.ComputeInverse();
                ins.m_bounds = wmo.m_bounds;
                ins.m_modelFilename = wmo.m_fileName;

                m_staticWmos.insert({static_cast<unsigned int>(wmo.m_id), ins});
            }
        }

        std::uint32_t doodadInstanceCount;
        in >> doodadInstanceCount;

        if (doodadInstanceCount)
        {
            std::vector<DoodadFileInstance> doodadInstances(doodadInstanceCount);
            in.ReadBytes(&doodadInstances[0], doodadInstanceCount * sizeof(DoodadFileInstance));

            for (auto const& doodad : doodadInstances)
            {
                DoodadInstance ins;

                ins.m_transformMatrix =
                    math::Matrix::CreateFromArray(doodad.m_transformMatrix, sizeof(doodad.m_transformMatrix) / sizeof(doodad.m_transformMatrix[0]));
                ins.m_inverseTransformMatrix = ins.m_transformMatrix.ComputeInverse();
                ins.m_bounds = doodad.m_bounds;
                ins.m_modelFilename = doodad.m_fileName;

                m_staticDoodads.insert({static_cast<unsigned int>(doodad.m_id), ins});
            }
        }
    }
    else
    {
        // no ADTs in this map
        ::memset(m_hasADT, 0, sizeof(m_hasADT));

        WmoFileInstance globalWmo;
        in >> globalWmo;

        WmoInstance ins;

        ins.m_doodadSet = globalWmo.m_doodadSet;
        ins.m_nameSet = globalWmo.m_nameSet;
        ins.m_transformMatrix =
            math::Matrix::CreateFromArray(globalWmo.m_transformMatrix, sizeof(globalWmo.m_transformMatrix) / sizeof(globalWmo.m_transformMatrix[0]));
        ins.m_inverseTransformMatrix = ins.m_transformMatrix.ComputeInverse();
        ins.m_bounds = globalWmo.m_bounds;

        auto model = EnsureWmoModelLoaded(globalWmo.m_fileName);
        ins.m_model = model;

        m_staticWmos.insert({GlobalWmoId, ins});

        dtNavMeshParams params;

        params.orig[0] = -globalWmo.m_bounds.MaxCorner.Y;
        params.orig[1] = globalWmo.m_bounds.MinCorner.Z;
        params.orig[2] = -globalWmo.m_bounds.MaxCorner.X;
        params.tileHeight = params.tileWidth = MeshSettings::TileSize;

        auto const tileWidth = static_cast<int>(::ceilf((globalWmo.m_bounds.MaxCorner.X - globalWmo.m_bounds.MinCorner.X) / MeshSettings::TileSize));
        auto const tileHeight = static_cast<int>(::ceilf((globalWmo.m_bounds.MaxCorner.Y - globalWmo.m_bounds.MinCorner.Y) / MeshSettings::TileSize));

        params.maxTiles = tileWidth * tileHeight;
        params.maxPolys = 1 << DT_POLY_BITS;

        auto const result = m_navMesh.init(&params);
        assert(result == DT_SUCCESS);

        auto const nav_path = m_dataPath / "Nav" / "Map.nav";

        utility::BinaryStream navIn(nav_path);

        navIn.Decompress();

        NavFileHeader header;
        navIn >> header;

        header.Verify(true);

        if (header.x != MeshSettings::WMOcoordinate || header.y != MeshSettings::WMOcoordinate)
            THROW("Incorrect WMO coordinates");

        for (auto i = 0u; i < header.tileCount; ++i)
        {
            auto tile = std::make_unique<Tile>(this, navIn, nav_path);

            // for a global wmo, all tiles are guarunteed to contain the model
            tile->m_staticWmos.push_back(GlobalWmoId);
            tile->m_staticWmoModels.push_back(model);

            m_tiles[{tile->m_x, tile->m_y}] = std::move(tile);
        }
    }

    if (m_navQuery.init(&m_navMesh, 65535) != DT_SUCCESS)
        THROW("dtNavMeshQuery::init failed");
}

std::shared_ptr<WmoModel> Map::LoadModelForWmoInstance(unsigned int instanceId)
{
    auto const instance = m_staticWmos.find(instanceId);

    // ensure it exists.  this should never fail
    if (instance == m_staticWmos.end())
        THROW("Unknown WMO instance requested");

    // if the model is loaded, return it
    if (!instance->second.m_model.expired())
        return instance->second.m_model.lock();

    auto model = EnsureWmoModelLoaded(instance->second.m_modelFilename);

    instance->second.m_model = model;

    return model;
}

std::shared_ptr<DoodadModel> Map::LoadModelForDoodadInstance(unsigned int instanceId)
{
    auto const instance = m_staticDoodads.find(instanceId);

    // ensure it exists.  this should never fail
    if (instance == m_staticDoodads.end())
        THROW("Unknown doodad instance requested");

    // if the model is loaded, return it
    if (!instance->second.m_model.expired())
        return instance->second.m_model.lock();

    auto model = EnsureDoodadModelLoaded(instance->second.m_modelFilename);

    instance->second.m_model = model;

    return model;
}

std::shared_ptr<DoodadModel> Map::EnsureDoodadModelLoaded(const std::string& mpq_path)
{
    auto const bvhFilename = m_bvhLoader.GetBVHPath(mpq_path);

    // if this model is currently loaded, return it
    auto const i = m_loadedDoodadModels.find(bvhFilename);
    if (i != m_loadedDoodadModels.end() && !i->second.expired())
        return i->second.lock();

    // else, load it
    utility::BinaryStream in(bvhFilename);

    auto model = std::make_shared<pathfind::DoodadModel>();

    if (!model->m_aabbTree.Deserialize(in))
        THROW("Could not deserialize doodad").ErrorCode();

    m_loadedDoodadModels[bvhFilename] = model;
    return std::move(model);
}

std::shared_ptr<WmoModel> Map::EnsureWmoModelLoaded(const std::string& mpq_path)
{
    auto const bvhFilename = m_bvhLoader.GetBVHPath(mpq_path);

    // if this model is currently loaded, return it
    auto const i = m_loadedWmoModels.find(bvhFilename);
    if (i != m_loadedWmoModels.end() && !i->second.expired())
        return i->second.lock();

    // else, load it
    utility::BinaryStream in(bvhFilename);

    auto model = std::make_shared<pathfind::WmoModel>();

    if (!model->m_aabbTree.Deserialize(in))
        THROW("Could not deserialize WMO").ErrorCode();

    std::uint32_t rootId, nameSetCount;
    in >> rootId >> nameSetCount;

    for (auto i = 0u; i < nameSetCount; ++i)
    {
        std::uint32_t nameSet, areaId, zoneId;
        in >> nameSet >> areaId >> zoneId;

        model->m_nameSetToAreaZone[nameSet] = { areaId, zoneId };
    }

    std::uint32_t doodadSetCount;
    in >> doodadSetCount;

    model->m_doodadSets.resize(doodadSetCount);
    model->m_loadedDoodadSets.resize(doodadSetCount);

    for (std::uint32_t set = 0; set < doodadSetCount; ++set)
    {
        std::uint32_t doodadSetSize;
        in >> doodadSetSize;

        model->m_doodadSets[set].resize(doodadSetSize);

        for (std::uint32_t doodad = 0; doodad < doodadSetSize; ++doodad)
        {
            float transformMatrix[16];
            in >> transformMatrix;

            model->m_doodadSets[set][doodad].m_transformMatrix =
                math::Matrix::CreateFromArray(transformMatrix, sizeof(transformMatrix) / sizeof(transformMatrix[0]));

            in >> model->m_doodadSets[set][doodad].m_bounds;

            char doodadFileName[MeshSettings::MaxMPQPathLength];
            in >> doodadFileName;

            auto doodadModel = EnsureDoodadModelLoaded(doodadFileName);

            // loaded doodads serve as reference counters for automatic unload
            model->m_loadedDoodadSets[set].push_back(doodadModel);
            model->m_doodadSets[set][doodad].m_model = m_loadedDoodadModels[doodadFileName] = doodadModel;
        }
    }

    m_loadedWmoModels[bvhFilename] = model;

    return std::move(model);
}

bool Map::HasADT(int x, int y) const
{
    return m_hasADT[x][y];
}

bool Map::IsADTLoaded(int x, int y) const
{
    return m_loadedADT[x][y];
}

bool Map::LoadADT(int x, int y)
{
    if (m_loadedADT[x][y])
        return true;

    if (!m_hasADT[x][y])
        return false;

    std::stringstream str;
    str << std::setfill('0') << std::setw(2) << x << "_" << std::setfill('0') << std::setw(2) << y << ".nav";

    auto const nav_path = m_dataPath / "Nav" / m_mapName / str.str();

    utility::BinaryStream stream(nav_path);

    stream.Decompress();

    NavFileHeader header;
    stream >> header;

    header.Verify(false);

    if (header.x != static_cast<std::uint32_t>(x) || header.y != static_cast<std::uint32_t>(y))
        THROW("Incorrect ADT coordinates");

    for (auto i = 0u; i < header.tileCount; ++i)
    {
        auto tile = std::make_unique<Tile>(this, stream, nav_path);
        m_tiles[{tile->m_x, tile->m_y}] = std::move(tile);
    }

    m_loadedADT[x][y] = true;

    return true;
}

void Map::UnloadADT(int x, int y)
{
    if (!m_loadedADT[x][y])
        return;

    for (auto tileY = y * MeshSettings::TilesPerADT; tileY < (y + 1) * MeshSettings::TilesPerADT; ++tileY)
        for (auto tileX = x * MeshSettings::TilesPerADT; tileX < (x + 1) * MeshSettings::TilesPerADT; ++tileX)
        {
            auto i = m_tiles.find({tileX, tileY});

            if (i != m_tiles.end())
                m_tiles.erase(i);
        }

    m_loadedADT[x][y] = false;
}

int Map::LoadAllADTs()
{
    int result = 0;

    for (auto y = 0; y < MeshSettings::Adts; ++y)
        for (auto x = 0; x < MeshSettings::Adts; ++x)
            if (m_hasADT[x][y] && LoadADT(x, y))
                ++result;

    return result;
}

std::shared_ptr<Model> Map::GetOrLoadModelByDisplayId(unsigned int displayId)
{
    // Get the BVH file for this display ID
    auto const bvh_path = m_bvhLoader.GetBVHPath(displayId);

    // TODO: add logic based on mpq_path
    auto const doodad = false;
    // auto const doodad = fileName[0] == 'd' || fileName[0] == 'D';

    if (doodad)
    {
        // auto const i = m_loadedDoodadModels.find(fileName);
        // if (i != m_loadedDoodadModels.end() && !i->second.expired())
        //    return i->second.lock();

        return EnsureDoodadModelLoaded(bvh_path);
    }
    else
    {
        // auto const i = m_loadedWmoModels.find(fileName);
        // if (i != m_loadedWmoModels.end() && !i->second.expired())
        //    return i->second.lock();

        return EnsureWmoModelLoaded(bvh_path);
    }

    // return nullptr;
}

bool Map::FindPath(const math::Vertex& start, const math::Vertex& end, std::vector<math::Vertex>& output, bool allowPartial) const
{
    constexpr float extents[] = {5.f, 5.f, 5.f};

    float recastStart[3];
    float recastEnd[3];

    math::Convert::VertexToRecast(start, recastStart);
    math::Convert::VertexToRecast(end, recastEnd);

    dtPolyRef startPolyRef, endPolyRef;
    if (!(m_navQuery.findNearestPoly(recastStart, extents, &m_queryFilter, &startPolyRef, nullptr) & DT_SUCCESS))
        return false;

    if (!startPolyRef)
        return false;

    if (!(m_navQuery.findNearestPoly(recastEnd, extents, &m_queryFilter, &endPolyRef, nullptr) & DT_SUCCESS))
        return false;

    if (!endPolyRef)
        return false;

    dtPolyRef polyRefBuffer[MaxPathHops];

    int pathLength;
    auto const findPathResult =
        m_navQuery.findPath(startPolyRef, endPolyRef, recastStart, recastEnd, &m_queryFilter, polyRefBuffer, &pathLength, MaxPathHops);
    if (!(findPathResult & DT_SUCCESS) || (!allowPartial && !!(findPathResult & DT_PARTIAL_RESULT)))
        return false;

    float pathBuffer[MaxPathHops * 3];
    auto const findStraightPathResult =
        m_navQuery.findStraightPath(recastStart, recastEnd, polyRefBuffer, pathLength, pathBuffer, nullptr, nullptr, &pathLength, MaxPathHops);
    if (!(findStraightPathResult & DT_SUCCESS) || (!allowPartial && !!(findStraightPathResult & DT_PARTIAL_RESULT)))
        return false;

    output.resize(pathLength);

    for (auto i = 0; i < pathLength; ++i)
        math::Convert::VertexToWow(&pathBuffer[i * 3], output[i]);

    return true;
}

bool Map::GetADTHeight(const Tile *tile, float x, float y, float &height,
    unsigned int *zone, unsigned int *area) const
{
    // check optional ADT quad height data for this tile
    if (tile->m_quadHeights.empty())
        return false;

    float northwestX, northwestY;
    math::Convert::TileToWorldNorthwestCorner(tile->m_x, tile->m_y, northwestX, northwestY);

    auto constexpr quadWidth = MeshSettings::AdtChunkSize / 8;

    // quad coordinates
    auto const quadX = static_cast<int>((northwestY - y) / quadWidth);
    auto const quadY = static_cast<int>((northwestX - x) / quadWidth);

    assert(quadX < 8 && quadY < 8);

    // if there is an ADT hole here, do not consider ADT height
    if (tile->m_quadHoles[quadX][quadY])
        return false;

    auto constexpr yMultiplier = 1 + 16 / MeshSettings::TilesPerChunk;
    auto constexpr midOffset = 1 + 8 / MeshSettings::TilesPerChunk;

    // compute the five quad vertices, with the following layout, in the recast coordinate system:
    // a   b
    //   c
    // d   e

    const float a[] = {
        -(northwestY - quadWidth * quadX), tile->m_quadHeights[yMultiplier * quadY + quadX], -(northwestX - quadWidth * quadY) };

    const float b[] = { -(northwestY - quadWidth * (quadX + 1)),
                        tile->m_quadHeights[yMultiplier * quadY + quadX + 1],
                        -(northwestX - quadWidth * quadY) };

    const float c[] = { -(northwestY - quadWidth * (quadX + 0.5f)),
                        tile->m_quadHeights[yMultiplier * quadY + quadX + midOffset],
                        -(northwestX - quadWidth * (quadY + 0.5f)) };

    const float d[] = { -(northwestY - quadWidth * quadX),
                        tile->m_quadHeights[yMultiplier * (quadY + 1) + quadX],
                        -(northwestX - quadWidth * (quadY + 1)) };

    const float e[] = { -(northwestY - quadWidth * (quadX + 1)),
                        tile->m_quadHeights[yMultiplier * (quadY + 1) + quadX + 1],
                        -(northwestX - quadWidth * (quadY + 1)) };

    // the point we want to intersect with each triangle
    const float p[] = { -y, (std::numeric_limits<float>::min)(), -x };

    float h;

    // this if-statement will ensure that at most one of these calls will succeed, which is what we want
    if (dtClosestHeightPointTriangle(p, a, b, c, h) || dtClosestHeightPointTriangle(p, b, e, c, h) ||
        dtClosestHeightPointTriangle(p, c, e, d, h) || dtClosestHeightPointTriangle(p, a, c, d, h))
    {
        height = h;
        if (area)
            *area = tile->m_areaId;
        if (zone)
            *zone = tile->m_zoneId;
        return true;
    }

    return false;
}

float Map::FindPreciseZ(float x, float y, float zHint) const
{
    float result = zHint;

    // zHint is assumed to be a value from the Detour detailed tri mesh, which has an error of +/- MeshSettings::DetailSampleMaxError.
    // because we want to ensure that zHint is above the 'true' value, we shift up this error range so that the error is
    // instead 0 <= error <= 2*MeshSettings::DetailSampleMaxError
    zHint += MeshSettings::DetailSampleMaxError;

    // find the tile corresponding to this (x, y)
    int tileX, tileY;
    math::Convert::WorldToTile({x, y, 0.f}, tileX, tileY);

    auto const tile = m_tiles.find({tileX, tileY});

    if (tile == m_tiles.end())
        THROW("Tile not found for requested (x, y)");

    // check BVH data for this tile
    // note that we assume the ground, if there is any, is within 2x
    bool rayHit;
    math::Ray ray{{x, y, zHint}, {x, y, zHint - 3.f * MeshSettings::DetailSampleMaxError}};
    if ((rayHit = RayCast(ray, {tile->second.get()})))
        result = ray.GetHitPoint().Z;

    float adtHeight;
    auto const adtHit = GetADTHeight(tile->second.get(), x, y, adtHeight);

    // one of these two should always hit, at least in places where this query is expected to be made
    assert(rayHit || adtHit);

    // if no object was hit but the adt was, we don't care about the current value of 'result'
    if (adtHit && (!rayHit || (adtHeight > result)))
        result = adtHeight;

    return result;
}

bool Map::FindHeights(float x, float y, std::vector<float>& output) const
{
    constexpr float extents[] = {0.f, (std::numeric_limits<float>::max)(), 0.f};
    float recastCenter[3];

    math::Convert::VertexToRecast({x, y, 0.f}, recastCenter);

    dtPolyRef polys[MaxStackedPolys];
    int polyCount;

    auto const queryResult = m_navQuery.queryPolygons(recastCenter, extents, &m_queryFilter, polys, &polyCount, MaxStackedPolys);
    assert(queryResult == DT_SUCCESS);

    output.reserve(polyCount);

    // FIXME: not sure what the use case for this search is.  should it be always precise, never, or user-defined?
    for (auto i = 0; i < polyCount; ++i)
    {
        float result;

        if (m_navQuery.getPolyHeight(polys[i], recastCenter, &result) == DT_SUCCESS)
            output.push_back(FindPreciseZ(x, y, result));
    }

    return !output.empty();
}

bool Map::ZoneAndArea(const math::Vertex& position, unsigned int& zone, unsigned int& area) const
{
    // find the tile corresponding to this (x, y)
    int tileX, tileY;
    math::Convert::WorldToTile({ position.X, position.Y, 0.f }, tileX, tileY);

    auto const tile = m_tiles.find({ tileX, tileY });

    if (tile == m_tiles.end())
        return false;

    std::vector<const Tile*> tiles{ tile->second.get() };

    math::Ray ray{
        {position.X, position.Y, position.Z},
        {position.X, position.Y, tile->second->m_bounds.getMinimum().Z}
    };

    unsigned int localZone, localArea;
    auto const rayResult = RayCast(ray, tiles, &localZone, &localArea);
    if (rayResult)
    {
        zone = localZone;
        area = localArea;
    }

    float adtHeight;
    unsigned int adtZone, adtArea;
    auto const adtResult = GetADTHeight(tile->second.get(), position.X, position.Y, adtHeight,
        &adtZone, &adtArea);

    if (adtResult && adtHeight > ray.GetHitPoint().Z)
    {
        zone = adtZone;
        area = adtArea;
    }

    assert(rayResult || adtResult);

    return rayResult || adtResult;
}

bool Map::RayCast(math::Ray& ray) const
{
    std::vector<const Tile*> tiles;

    // find affected tiles
    for (auto const& tile : m_tiles)
        if (ray.IntersectBoundingBox(tile.second->m_bounds))
            tiles.push_back(tile.second.get());

    return RayCast(ray, tiles);
}

bool Map::RayCast(math::Ray& ray, const std::vector<const Tile*>& tiles,
    unsigned int* zone, unsigned int* area) const
{
    auto const start = ray.GetStartPoint();
    auto const end = ray.GetEndPoint();

    auto hit = false;

    // FIXME: Examine WoW++ functions Map::isInLineOfSight and forEachTileInRayXY() to see if that approach is more performant

    // save ids to prevent repeated checks on the same objects
    std::unordered_set<std::uint32_t> staticWmos, staticDoodads;
    std::unordered_set<std::uint64_t> temporaryWmos, temporaryDoodads;

    // for each tile...
    for (auto const tile : tiles)
    {
        // if the tile itself does not intersect our ray, do nothing
        if (!ray.IntersectBoundingBox(tile->m_bounds))
            continue;

        // measure intersection for all static wmos on the tile
        for (auto const& id : tile->m_staticWmos)
        {
            // skip static wmos we have already seen (possibly from a previous tile)
            if (staticWmos.find(id) != staticWmos.end())
                continue;

            // record this static wmo as having been tested
            staticWmos.insert(id);

            auto const& instance = m_staticWmos.at(id);

            // skip this wmo if the bbox doesn't intersect, saves us from calculating the inverse ray
            if (!ray.IntersectBoundingBox(instance.m_bounds))
                continue;

            math::Ray rayInverse(math::Vector3::Transform(start, instance.m_inverseTransformMatrix),
                                 math::Vector3::Transform(end, instance.m_inverseTransformMatrix));

            // if this is a closer hit, update the original ray's distance
            if (auto model = instance.m_model.lock())
            {
                if (model->m_aabbTree.IntersectRay(rayInverse) && rayInverse.GetDistance() < ray.GetDistance())
                {
                    hit = true;
                    ray.SetHitPoint(rayInverse.GetDistance());

                    if (area)
                        *area = model->m_nameSetToAreaZone[instance.m_nameSet].first;
                    if (zone)
                        *zone = model->m_nameSetToAreaZone[instance.m_nameSet].second;
                }
            }
        }

        // measure intersection for all static doodads on this tile
        for (auto const& id : tile->m_staticDoodads)
        {
            // skip static doodads we have already seen (possibly from a previous tile)
            if (staticDoodads.find(id) != staticDoodads.end())
                continue;

            // record this static doodad as having been tested
            staticDoodads.insert(id);

            auto const& instance = m_staticDoodads.at(id);

            // skip this doodad if the bbox doesn't intersect, saves us from calculating the inverse ray
            if (!ray.IntersectBoundingBox(instance.m_bounds))
                continue;

            math::Ray rayInverse(math::Vector3::Transform(start, instance.m_inverseTransformMatrix),
                                 math::Vector3::Transform(end, instance.m_inverseTransformMatrix));

            // if this is a closer hit, update the original ray's distance
            if (instance.m_model.lock()->m_aabbTree.IntersectRay(rayInverse) && rayInverse.GetDistance() < ray.GetDistance())
            {
                hit = true;
                ray.SetHitPoint(rayInverse.GetDistance());
            }
        }

        // measure intersection for all temporary wmos on this tile
        for (auto const& wmo : tile->m_temporaryWmos)
        {
            // skip static wmos we have already seen (possibly from a previous tile)
            if (temporaryWmos.find(wmo.first) != temporaryWmos.end())
                continue;

            // record this temporary wmo as having been tested
            temporaryWmos.insert(wmo.first);

            // skip this wmo if the bbox doesn't intersect, saves us from calculating the inverse ray
            if (!ray.IntersectBoundingBox(wmo.second->m_bounds))
                continue;

            math::Ray rayInverse(math::Vector3::Transform(start, wmo.second->m_inverseTransformMatrix),
                                 math::Vector3::Transform(end, wmo.second->m_inverseTransformMatrix));

            // if this is a closer hit, update the original ray's distance
            if (auto model = wmo.second->m_model.lock())
            {
                if (model->m_aabbTree.IntersectRay(rayInverse) && rayInverse.GetDistance() < ray.GetDistance())
                {
                    hit = true;
                    ray.SetHitPoint(rayInverse.GetDistance());
                    if (area)
                        *area = model->m_nameSetToAreaZone[wmo.second->m_nameSet].first;
                    if (zone)
                        *zone = model->m_nameSetToAreaZone[wmo.second->m_nameSet].second;
                }
            }
        }

        // measure intersection for all temporary doodads on this tile
        for (auto const& doodad : tile->m_temporaryDoodads)
        {
            // skip static wmos we have already seen (possibly from a previous tile)
            if (temporaryDoodads.find(doodad.first) != temporaryDoodads.end())
                continue;

            // record this temporary wmo as having been tested
            temporaryDoodads.insert(doodad.first);

            // skip this doodad if the bbox doesn't intersect, saves us from calculating the inverse ray
            if (!ray.IntersectBoundingBox(doodad.second->m_bounds))
                continue;

            math::Ray rayInverse(math::Vector3::Transform(start, doodad.second->m_inverseTransformMatrix),
                                 math::Vector3::Transform(end, doodad.second->m_inverseTransformMatrix));

            // if this is a closer hit, update the original ray's distance
            if (doodad.second->m_model.lock()->m_aabbTree.IntersectRay(rayInverse) && rayInverse.GetDistance() < ray.GetDistance())
            {
                hit = true;
                ray.SetHitPoint(rayInverse.GetDistance());
            }
        }
    }

    return hit;
}
} // namespace pathfind
