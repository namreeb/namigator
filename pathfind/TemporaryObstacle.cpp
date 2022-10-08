#include "Map.hpp"
#include "MapBuilder/MeshBuilder.hpp"
#include "Tile.hpp"
#include "recastnavigation/Detour/Include/DetourNavMeshBuilder.h"
#include "recastnavigation/Recast/Include/Recast.h"
#include "utility/BoundingBox.hpp"
#include "utility/Exception.hpp"
#include "utility/MathHelper.hpp"
#include "utility/Vector.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <thread>

namespace
{
class RecastContext : public rcContext
{
private:
    const rcLogCategory m_logLevel;

    virtual void doLog(const rcLogCategory category, const char* msg,
                       const int) override
    {
        if (!m_logLevel || category < m_logLevel)
            return;

        std::stringstream out;

        out << "Thread #" << std::setfill(' ') << std::setw(6)
            << std::this_thread::get_id() << " ";

        switch (category)
        {
            case rcLogCategory::RC_LOG_ERROR:
                out << "ERROR: ";
                break;
            case rcLogCategory::RC_LOG_PROGRESS:
                out << "PROGRESS: ";
                break;
            case rcLogCategory::RC_LOG_WARNING:
                out << "WARNING: ";
                break;
            default:
                out << "rcContext::doLog(" << category << "): ";
                break;
        }

        out << msg;

        THROW_MSG("Recast Failure", Result::RECAST_FAILURE).Message(out.str());
    }

public:
    RecastContext(int logLevel)
        : m_logLevel(static_cast<rcLogCategory>(logLevel))
    {
    }
};

#define ZERO(x) memset(&x, 0, sizeof(x))

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

// TODO: Combine with MeshBuilder.cpp version?

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

bool RebuildMeshTile(rcContext& ctx, const rcConfig& config, int tileX,
                     int tileY, rcHeightfield& solid,
                     std::vector<unsigned char>& out)
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

    out.clear();
    out.resize(static_cast<size_t>(outDataSize));
    memcpy(&out[0], outData, out.size());

    dtFree(outData);

    return true;
}
} // namespace

namespace pathfind
{
void Map::AddGameObject(std::uint64_t guid, unsigned int displayId,
                        const math::Vector3& position, float orientation,
                        int doodadSet)
{
    auto const matrix = math::Matrix::CreateRotationZ(orientation);
    AddGameObject(guid, displayId, position, matrix, doodadSet);
}

void Map::AddGameObject(std::uint64_t guid, unsigned int displayId,
                        const math::Vector3& position,
                        const math::Quaternion& rotation, int doodadSet)
{
    auto const matrix = math::Matrix::CreateFromQuaternion(rotation);
    AddGameObject(guid, displayId, position, matrix, doodadSet);
}

void Map::AddGameObject(std::uint64_t guid, unsigned int displayId,
                        const math::Vector3& position,
                        const math::Matrix& rotation, int /*doodadSet*/)
{
    if (m_temporaryDoodads.find(guid) != m_temporaryDoodads.end() ||
        m_temporaryWmos.find(guid) != m_temporaryWmos.end())
        THROW(Result::GAMEOBJECT_WITH_SPECIFIED_GUID_ALREADY_EXISTS);

    auto const matrix =
        math::Matrix::CreateTranslationMatrix(position) * rotation;

    auto const bvh_path = m_bvhLoader.GetBVHPath(displayId);
    // TODO: Add logic based on bvh_path
    auto const doodad = true;
    // auto const doodad = m_temporaryObstaclePaths[displayId][0] == 'd' ||
    // m_temporaryObstaclePaths[displayId][0] == 'D';

    if (doodad)
    {
        auto instance = std::make_shared<DoodadInstance>();

        instance->m_transformMatrix = matrix;
        instance->m_inverseTransformMatrix = matrix.ComputeInverse();
        instance->m_modelFilename = bvh_path;
        auto model = EnsureDoodadModelLoaded(bvh_path);
        instance->m_model = model;

        instance->m_translatedVertices.reserve(
            model->m_aabbTree.Vertices().size());

        for (auto const& v : model->m_aabbTree.Vertices())
            instance->m_translatedVertices.emplace_back(
                math::Vector3::Transform(v, matrix));

        // models are guarunteed to have more than zero vertices
        math::BoundingBox bounds {instance->m_translatedVertices[0],
                                  instance->m_translatedVertices[0]};

        for (auto i = 1u; i < instance->m_translatedVertices.size(); ++i)
            bounds.update(instance->m_translatedVertices[i]);

        instance->m_bounds = bounds;
        m_temporaryDoodads[guid] = instance;

        for (auto const& tile : m_tiles)
        {
            if (!tile.second->m_bounds.intersect2d(instance->m_bounds))
                continue;

            tile.second->AddTemporaryDoodad(guid, instance);
        }
    }
    else
    {
        THROW(Result::TEMPORARY_WMO_OBSTACLES_ARE_NOT_SUPPORTED);

        // auto const model = LoadWmoModel(0); // TODO make this support loading
        // a filename

        //// if there is only one, the specified set is irrelevant.  use it!
        // if (doodadSet < 0 && model->m_doodadSets.size() > 1)
        //    THROW(NO_DOODAD_SET_SPECIFIED_FOR_WMO_GAME_OBJECT);
        //
        // if (doodadSet < 0)
        //    doodadSet = 0;

        // WmoInstance instance { static_cast<unsigned short>(doodadSet),
        // matrix, matrix.ComputeInverse(), math::BoundingBox(), model };
    }
}

void Tile::AddTemporaryDoodad(std::uint64_t guid,
                              std::shared_ptr<DoodadInstance> doodad)
{
    if (!m_heightField.spans)
        LoadHeightField();

    auto const model = doodad->m_model.lock();

    std::vector<float> recastVertices;
    math::Convert::VerticesToRecast(doodad->m_translatedVertices,
                                    recastVertices);

    std::vector<unsigned char> areas(model->m_aabbTree.Indices().size());

    m_temporaryDoodads[guid] = std::move(doodad);

    RecastContext ctx(rcLogCategory::RC_LOG_ERROR);
    rcClearUnwalkableTriangles(
        &ctx, MeshSettings::WalkableSlope, &recastVertices[0],
        static_cast<int>(recastVertices.size() / 3),
        &model->m_aabbTree.Indices()[0],
        static_cast<int>(model->m_aabbTree.Indices().size() / 3), &areas[0]);
    rcRasterizeTriangles(
        &ctx, &recastVertices[0], static_cast<int>(recastVertices.size() / 3),
        &model->m_aabbTree.Indices()[0], &areas[0],
        static_cast<int>(model->m_aabbTree.Indices().size() / 3),
        m_heightField);

    // we don't want to filter ledge spans from ADT terrain.  this will restore
    // the area for these spans, which we are using for flags
    {
        std::vector<std::pair<rcSpan*, unsigned int>> groundSpanAreas;

        groundSpanAreas.reserve(m_heightField.width * m_heightField.height);

        for (auto i = 0; i < m_heightField.width * m_heightField.height; ++i)
            for (rcSpan* s = m_heightField.spans[i]; s; s = s->next)
                if (!!(s->area & PolyFlags::Ground))
                    groundSpanAreas.push_back(std::pair<rcSpan*, unsigned int>(
                        s, static_cast<unsigned int>(s->area)));

        rcFilterLedgeSpans(&ctx, MeshSettings::VoxelWalkableHeight,
                           MeshSettings::VoxelWalkableClimb, m_heightField);

        for (auto p : groundSpanAreas)
            p.first->area = p.second;
    }

    rcFilterWalkableLowHeightSpans(&ctx, MeshSettings::VoxelWalkableHeight,
                                   m_heightField);
    rcFilterLowHangingWalkableObstacles(&ctx, MeshSettings::VoxelWalkableClimb,
                                        m_heightField);

    rcConfig config;

    InitializeRecastConfig(config);

    // build the mesh into a secondary buffer, rather than overwriting the
    // previous tile, so that we can delay the old tile's removal
    std::vector<std::uint8_t> newTileData;
    auto const buildResult =
        RebuildMeshTile(ctx, config, m_x, m_y, m_heightField, newTileData);
    assert(buildResult);

    if (m_ref)
    {
        auto const removeResult =
            m_map->m_navMesh.removeTile(m_ref, nullptr, nullptr);
        assert(removeResult == DT_SUCCESS);
    }

    m_tileData = std::move(newTileData);

    auto const insertResult = m_map->m_navMesh.addTile(
        &m_tileData[0], static_cast<int>(m_tileData.size()), 0, m_ref, &m_ref);

    assert(insertResult == DT_SUCCESS);
}
} // namespace pathfind
