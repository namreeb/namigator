#include "Map.hpp"
#include "Tile.hpp"

#include "RecastDetourBuild/Include/Common.hpp"
#include "utility/Include/MathHelper.hpp"
#include "utility/Include/Exception.hpp"
#include "utility/Include/Ray.hpp"

#include "Detour/Include/DetourNavMesh.h"
#include "Detour/Include/DetourNavMeshQuery.h"

#include <string>
#include <sstream>
#include <fstream>
#include <cassert>
#include <cstdint>
#include <vector>
#include <iomanip>
#include <list>
#include <limits>
#include <algorithm>
#include <unordered_set>

static_assert(sizeof(char) == 1, "char must be one byte");

#pragma pack(push, 1)
struct WmoFileInstance
{
    std::uint32_t m_id;
    std::uint16_t m_doodadSet;
    float m_transformMatrix[16];
    utility::BoundingBox m_bounds;
    char m_fileName[64];
};

struct DoodadFileInstance
{
    std::uint32_t m_id;
    float m_transformMatrix[16];
    utility::BoundingBox m_bounds;
    char m_fileName[64];
};

struct NavFileHeader
{
    std::uint32_t sig;
    std::uint32_t ver;
    std::uint32_t kind;
    std::uint32_t x;
    std::uint32_t y;
    std::uint32_t tileCount;

    void Verify(bool wmo) const
    {
        if (sig != MeshSettings::FileSignature)
            THROW("Incorrect file signature");

        if (ver != MeshSettings::FileVersion)
            THROW("Incorrect file version");

        if (wmo)
        {
            if (kind != MeshSettings::FileWMO)
                THROW("Not WMO nav file");

            if (x != MeshSettings::WMOcoordinate || y != MeshSettings::WMOcoordinate)
                THROW("Not WMO tile coordinates");
        }
        
        if (!wmo && kind != MeshSettings::FileADT)
            THROW("Not ADT nav file");
    }
};
#pragma pack(pop)

namespace
{
// TODO this function can probably be discarded once support is added for using heightfield to add temporary obstacles
void SkipHeightField(utility::BinaryStream &stream)
{
    std::int32_t width, height;
    stream >> width >> height;

    stream.rpos(stream.rpos() + 32);

    std::uint32_t size;
    stream >> size;

    stream.rpos(stream.rpos() + size);
}
}

namespace pathfind
{
Map::Map(const std::string &dataPath, const std::string &mapName) : m_dataPath(dataPath), m_mapName(mapName)
{
    std::stringstream continentFile;

    continentFile << dataPath << "\\" << mapName << ".map";

    std::ifstream in(continentFile.str(), std::ifstream::binary);
	if (!in)
		THROW("Could not open map file");

    std::uint32_t magic;
    in.read(reinterpret_cast<char *>(&magic), sizeof(magic));

    if (magic != 'MAP1')
        THROW("Invalid map file");

    std::uint8_t hasTerrain;
    in.read(reinterpret_cast<char *>(&hasTerrain), sizeof(hasTerrain));

    if (hasTerrain)
    {
        dtNavMeshParams params;

        constexpr float tileSize = -32.f * MeshSettings::AdtSize;
        constexpr int maxTiles = MeshSettings::TileCount * MeshSettings::TileCount;

        params.orig[0] = tileSize;
        params.orig[1] = 0.f;
        params.orig[2] = tileSize;
        params.tileHeight = params.tileWidth = MeshSettings::TileSize;
        params.maxTiles = maxTiles;
        params.maxPolys = 1 << DT_POLY_BITS;

        auto const result = m_navMesh.init(&params);
        assert(result == DT_SUCCESS);

        std::uint32_t wmoInstanceCount;
        in.read(reinterpret_cast<char *>(&wmoInstanceCount), sizeof(wmoInstanceCount));

        if (wmoInstanceCount)
        {
            std::vector<WmoFileInstance> wmoInstances(wmoInstanceCount);
            in.read(reinterpret_cast<char *>(&wmoInstances[0]), wmoInstanceCount*sizeof(WmoFileInstance));

            for (auto const &wmo : wmoInstances)
            {
                WmoInstance ins;

                ins.m_doodadSet = static_cast<unsigned short>(wmo.m_doodadSet);
                ins.m_transformMatrix = utility::Matrix::CreateFromArray(wmo.m_transformMatrix, sizeof(wmo.m_transformMatrix) / sizeof(wmo.m_transformMatrix[0]));
                ins.m_inverseTransformMatrix = ins.m_transformMatrix.ComputeInverse();
                ins.m_bounds = wmo.m_bounds;
                ins.m_fileName = wmo.m_fileName;

                m_wmoInstances.insert({ static_cast<unsigned int>(wmo.m_id), ins });
            }
        }

        std::uint32_t doodadInstanceCount;
        in.read(reinterpret_cast<char *>(&doodadInstanceCount), sizeof(doodadInstanceCount));

        if (doodadInstanceCount)
        {
            std::vector<DoodadFileInstance> doodadInstances(doodadInstanceCount);
            in.read(reinterpret_cast<char *>(&doodadInstances[0]), doodadInstanceCount*sizeof(DoodadFileInstance));

            for (auto const &doodad : doodadInstances)
            {
                DoodadInstance ins;

                ins.m_transformMatrix = utility::Matrix::CreateFromArray(doodad.m_transformMatrix, sizeof(doodad.m_transformMatrix) / sizeof(doodad.m_transformMatrix[0]));
                ins.m_inverseTransformMatrix = ins.m_transformMatrix.ComputeInverse();
                ins.m_bounds = doodad.m_bounds;
                ins.m_fileName = doodad.m_fileName;

                m_doodadInstances.insert({ static_cast<unsigned int>(doodad.m_id), ins });
            }
        }
    }
    else
    {
        WmoFileInstance globalWmo;
        in.read(reinterpret_cast<char *>(&globalWmo), sizeof(globalWmo));

        WmoInstance ins;

        ins.m_doodadSet = static_cast<unsigned short>(globalWmo.m_doodadSet);
        ins.m_transformMatrix = utility::Matrix::CreateFromArray(globalWmo.m_transformMatrix, sizeof(globalWmo.m_transformMatrix) / sizeof(globalWmo.m_transformMatrix[0]));
        ins.m_inverseTransformMatrix = ins.m_transformMatrix.ComputeInverse();
        ins.m_bounds = globalWmo.m_bounds;
        ins.m_fileName = globalWmo.m_fileName;

        m_wmoInstances.insert({ GlobalWmoId, ins });
        m_globalWmo = LoadWmoModel(GlobalWmoId);

        dtNavMeshParams params;

        params.orig[0] = -globalWmo.m_bounds.MaxCorner.Y;
        params.orig[1] =  globalWmo.m_bounds.MinCorner.Z;
        params.orig[2] = -globalWmo.m_bounds.MaxCorner.X;
        params.tileHeight = params.tileWidth = MeshSettings::TileSize;

        auto const tileWidth = static_cast<int>(std::ceilf((globalWmo.m_bounds.MaxCorner.X - globalWmo.m_bounds.MinCorner.X) / MeshSettings::TileSize));
        auto const tileHeight = static_cast<int>(std::ceilf((globalWmo.m_bounds.MaxCorner.Y - globalWmo.m_bounds.MinCorner.Y) / MeshSettings::TileSize));

        params.maxTiles = tileWidth*tileHeight;
        params.maxPolys = 1 << DT_POLY_BITS;

        auto const result = m_navMesh.init(&params);
        assert(result == DT_SUCCESS);
        
        std::stringstream str;
        str << m_dataPath << "\\Nav\\" << m_mapName << "\\Map.nav";
        std::ifstream navFile(str.str(), std::ifstream::binary);
        utility::BinaryStream navIn(navFile);
        navFile.close();

        try
        {
            NavFileHeader header;
            navIn >> header;

            header.Verify(!!m_globalWmo);

            if (header.x != MeshSettings::WMOcoordinate || header.y != MeshSettings::WMOcoordinate)
                THROW("Incorrect WMO coordinates");

            for (auto i = 0u; i < header.tileCount; ++i)
            {
                auto tile = std::make_unique<Tile>(this, navIn);
                m_tiles[{tile->X(), tile->Y()}] = std::move(tile);
            }
        }
        catch (std::domain_error const &e)
        {
            std::stringstream err;
            err << "Failed to read navmesh file: " << e.what();
            MessageBox(nullptr, err.str().c_str(), "ERROR", 0);
        }
    }

    if (m_navQuery.init(&m_navMesh, 2048) != DT_SUCCESS)
        THROW("dtNavMeshQuery::init failed");
}

std::shared_ptr<WmoModel> Map::LoadWmoModel(unsigned int id)
{
    auto const instance = m_wmoInstances.find(id);

    // ensure it exists.  this should never fail
    if (instance == m_wmoInstances.end())
        THROW("Unknown WMO instance requested");

    auto model = m_loadedWmoModels[instance->second.m_fileName].lock();

    // if the model is not already loaded, load it
    if (!model)
    {
        std::stringstream file;
        file << m_dataPath << "\\BVH\\WMO_" << instance->second.m_fileName << ".bvh";

        std::ifstream in(file.str(), std::ifstream::binary);

        if (in.fail())
            THROW("Could not open WMO BVH file").ErrorCode();

        model = std::make_shared<WmoModel>();

        if (!model->m_aabbTree.Deserialize(in))
            THROW("Could not deserialize WMO").ErrorCode();

        std::uint32_t doodadSetCount;
        in.read(reinterpret_cast<char *>(&doodadSetCount), sizeof(doodadSetCount));

        model->m_doodadSets.resize(static_cast<size_t>(doodadSetCount));
        model->m_loadedDoodadSets.resize(static_cast<size_t>(doodadSetCount));

        for (std::uint32_t set = 0; set < doodadSetCount; ++set)
        {
            std::uint32_t doodadSetSize;
            in.read(reinterpret_cast<char *>(&doodadSetSize), sizeof(doodadSetSize));

            model->m_doodadSets[set].resize(doodadSetSize);

            for (std::uint32_t doodad = 0; doodad < doodadSetSize; ++doodad)
            {
                float transformMatrix[16];
                in.read(reinterpret_cast<char *>(transformMatrix), sizeof(transformMatrix));

                model->m_doodadSets[set][doodad].m_transformMatrix = utility::Matrix::CreateFromArray(transformMatrix, sizeof(transformMatrix) / sizeof(transformMatrix[0]));

                in >> model->m_doodadSets[set][doodad].m_bounds;

                char fileName[64];
                in.read(fileName, sizeof(fileName));
                model->m_doodadSets[set][doodad].m_fileName = fileName;
            }
        }

        m_loadedWmoModels[instance->second.m_fileName] = model;
    }

    auto const doodadSet = instance->second.m_doodadSet;

    // by now, we know that the model is loaded.  ensure that the doodad set for this instance is also loaded
    for (auto const &doodad : model->m_doodadSets[doodadSet])
    {
        auto doodadModel = LoadDoodadModel(doodad.m_fileName);

        model->m_loadedDoodadSets[doodadSet].push_back(doodadModel);
    }

    return model;
}

// lookup already loaded information about this doodad instance, and ensure that the AABB tree for the model is loaded
std::shared_ptr<DoodadModel> Map::LoadDoodadModel(unsigned int id)
{
    auto const instance = m_doodadInstances.find(id);

    // ensure it exists.  this should never fail
    if (instance == m_doodadInstances.end())
        THROW("Unknown doodad instance requested");

    return LoadDoodadModel(instance->second.m_fileName);
}

std::shared_ptr<DoodadModel> Map::LoadDoodadModel(const std::string &fileName)
{
    auto model = m_loadedDoodadModels[fileName].lock();

    if (!model)
    {
        std::stringstream file;
        file << m_dataPath << "\\BVH\\Doodad_" << fileName << ".bvh";

        std::ifstream in(file.str(), std::ifstream::binary);

        if (in.fail())
            return nullptr;

        model = std::make_shared<DoodadModel>();

        if (!model->m_aabbTree.Deserialize(in))
            THROW("Could not deserialize doodad").ErrorCode();

        m_loadedDoodadModels[fileName] = model;
    }

    return model;
}

void Map::LoadAllTiles()
{
    if (m_globalWmo)
    {
        
    }
    else
        for (auto y = 0; y < MeshSettings::Adts; ++y)
            for (auto x = 0; x < MeshSettings::Adts; ++x)
                LoadADT(x, y);
}

bool Map::LoadADT(int x, int y)
{
    // if any of the tiles for this ADT are loaded, it is already loaded
    for (auto tileY = y * MeshSettings::TilesPerADT; tileY < (y + 1)*MeshSettings::TilesPerADT; ++tileY)
        for (auto tileX = x * MeshSettings::TilesPerADT; tileX < (x + 1)*MeshSettings::TilesPerADT; ++tileX)
            if (m_tiles.find({ x, y }) != m_tiles.end())
                return true;

    std::stringstream str;
    str << m_dataPath << "\\Nav\\" << m_mapName << "\\"
        << std::setfill('0') << std::setw(4) << x << "_"
        << std::setfill('0') << std::setw(4) << y << ".nav";

    std::ifstream in(str.str(), std::ifstream::binary);
    utility::BinaryStream stream(in);
    in.close();

    try
    {
        NavFileHeader header;
        stream >> header;

        header.Verify(!!m_globalWmo);

        if (header.x != static_cast<std::uint32_t>(x) || header.y != static_cast<std::uint32_t>(y))
            THROW("Incorrect ADT coordinates");

        for (auto i = 0u; i < header.tileCount; ++i)
        {
            auto tile = std::make_unique<Tile>(this, stream);
            m_tiles[{tile->X(), tile->Y()}] = std::move(tile);
        }
    }
    catch (std::domain_error const&)
    {
        return false;
    }

    return true;
}

void Map::UnloadADT(int x, int y)
{
    for (auto tileY = y * MeshSettings::TilesPerADT; tileY < (y + 1)*MeshSettings::TilesPerADT; ++tileY)
        for (auto tileX = x * MeshSettings::TilesPerADT; tileX < (x + 1)*MeshSettings::TilesPerADT; ++tileX)
        {
            auto i = m_tiles.find({ tileX, tileY });

            if (i != m_tiles.end())
                m_tiles.erase(i);
        }
}

bool Map::FindPath(const utility::Vertex &start, const utility::Vertex &end, std::vector<utility::Vertex> &output, bool allowPartial) const
{
    constexpr float extents[] = { 5.f, 5.f, 5.f };

    float recastStart[3];
    float recastEnd[3];

    utility::Convert::VertexToRecast(start, recastStart);
    utility::Convert::VertexToRecast(end, recastEnd);

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
    auto const findPathResult = m_navQuery.findPath(startPolyRef, endPolyRef, recastStart, recastEnd, &m_queryFilter, polyRefBuffer, &pathLength, MaxPathHops);
    if (!(findPathResult & DT_SUCCESS) || (!allowPartial && !!(findPathResult & DT_PARTIAL_RESULT)))
        return false;

    float pathBuffer[MaxPathHops*3];
    auto const findStraightPathResult = m_navQuery.findStraightPath(recastStart, recastEnd, polyRefBuffer, pathLength, pathBuffer, nullptr, nullptr, &pathLength, MaxPathHops);
    if (!(findStraightPathResult & DT_SUCCESS) || (!allowPartial && !!(findStraightPathResult & DT_PARTIAL_RESULT)))
        return false;

    output.clear();
    output.resize(pathLength);

    for (auto i = 0; i < pathLength; ++i)
        utility::Convert::VertexToWow(&pathBuffer[i * 3], output[i]);

    return true;
}

bool Map::FindHeights(const utility::Vertex &position, std::vector<float> &output) const
{
    return FindHeights(position.X, position.Y, output);
}

bool Map::FindHeights(float x, float y, std::vector<float> &output) const
{
    const utility::Vertex stop(x, y, std::numeric_limits<float>::lowest());
    utility::Ray ray({ x, y, (std::numeric_limits<float>::max)() }, stop);

    output.clear();

    for (;;)
    {
        auto const ret = RayCast(ray);

        if (!ret)
            break;

        auto const &hitPoint = ray.GetHitPoint();

        output.push_back(hitPoint.Z);

        ray = utility::Ray(hitPoint, stop);
    }

    return output.size() > 0;
}

bool Map::RayCast(utility::Ray &ray) const
{
    int x1, x2, y1, y2;

    auto const start = ray.GetStartPoint();
    auto const end = ray.GetEndPoint();

    if (m_globalWmo)
    {
        auto const &wmoInstance = m_wmoInstances.at(GlobalWmoId);

        if (!ray.IntersectBoundingBox(wmoInstance.m_bounds))
            return false;

        utility::Ray rayInverse(utility::Vertex::Transform(start, wmoInstance.m_inverseTransformMatrix),
                                utility::Vertex::Transform(end,   wmoInstance.m_inverseTransformMatrix));

        bool hit = false;

        // does it intersect the global WMO itself?
        if (m_globalWmo->m_aabbTree.IntersectRay(rayInverse))
        {
            hit = true;
            ray.SetHitPoint(rayInverse.GetDistance());
        }

        // also check the doodads spawned by this WMO
        for (auto const &doodad : m_globalWmo->m_loadedDoodadSets[wmoInstance.m_doodadSet])
            if (doodad->m_aabbTree.IntersectRay(rayInverse) && rayInverse.GetDistance() < ray.GetDistance())
            {
                hit = true;
                ray.SetHitPoint(rayInverse.GetDistance());
            }

        return hit;
    }

    utility::Convert::WorldToAdt(start, x1, y1);
    utility::Convert::WorldToAdt(end, x2, y2);

    const int xStart = (std::min)(x1, x2);
    const int xStop  = (std::max)(x1, x2);
    const int yStart = (std::min)(y1, y2);
    const int yStop  = (std::max)(y1, y2);

    std::unordered_set<unsigned int> testedDoodads;
    std::unordered_set<unsigned int> testedWmos;

    // for all tiles involved
    for (int y = yStart; y <= yStop; ++y)
        for (int x = xStart; x <= xStop; ++x)
        {
            auto const itr = m_tiles.find({ x, y });
            if (itr == m_tiles.end())
                continue;

            auto const tile = (*itr).second.get();

            // check doodads for this tile
            for (auto const doodad : tile->m_doodads)
            {
                // if we've already tested it, skip it
                if (testedDoodads.find(doodad) != testedDoodads.end())
                    continue;

                testedDoodads.insert(doodad);

                auto const &doodadInstance = m_doodadInstances.at(doodad);

                // skip collision check if the ray doesnt intersect this model's bounding box
                if (!ray.IntersectBoundingBox(doodadInstance.m_bounds))
                    continue;

                auto const model = m_loadedDoodadModels.at(doodadInstance.m_fileName).lock();

                utility::Ray rayInverse(utility::Vertex::Transform(start, doodadInstance.m_inverseTransformMatrix),
                                        utility::Vertex::Transform(end,   doodadInstance.m_inverseTransformMatrix));

                // if there is a hit and it is closer than any previous hits, save it
                if (model->m_aabbTree.IntersectRay(rayInverse) && rayInverse.GetDistance() < ray.GetDistance())
                    ray.SetHitPoint(rayInverse.GetDistance());
            }

            // check WMOs for this tile
            for (auto const wmo : tile->m_wmos)
            {
                // if we've already tested it, skip it
                if (testedWmos.find(wmo) != testedWmos.end())
                    continue;

                testedWmos.insert(wmo);

                auto const &wmoInstance = m_wmoInstances.at(wmo);

                // skip collision check if the ray doesnt intersectthis model's bounding box
                if (!ray.IntersectBoundingBox(wmoInstance.m_bounds))
                    continue;

                auto const model = m_loadedWmoModels.at(wmoInstance.m_fileName).lock();

                utility::Ray rayInverse(utility::Vertex::Transform(start, wmoInstance.m_inverseTransformMatrix),
                                        utility::Vertex::Transform(end,   wmoInstance.m_inverseTransformMatrix));

                // first, check against the WMO itself

                // if there is a hit and it is closer than any previous hits, save it
                if (model->m_aabbTree.IntersectRay(rayInverse) && rayInverse.GetDistance() < ray.GetDistance())
                    ray.SetHitPoint(rayInverse.GetDistance());

                // second, check against the doodads from this WMO instance
                for (auto const &doodad : model->m_loadedDoodadSets[wmoInstance.m_doodadSet])
                    if (doodad->m_aabbTree.IntersectRay(rayInverse) && rayInverse.GetDistance() < ray.GetDistance())
                        ray.SetHitPoint(rayInverse.GetDistance());
            }
        }

    return true;
}
}