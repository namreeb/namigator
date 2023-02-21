#pragma once

#include "BVH.hpp"
#include "Common.hpp"
#include "Model.hpp"
#include "Tile.hpp"
#include "recastnavigation/Detour/Include/DetourNavMesh.h"
#include "recastnavigation/Detour/Include/DetourNavMeshQuery.h"
#include "utility/Ray.hpp"
#include "utility/Vector.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace std
{
template <>
struct hash<std::pair<int, int>>
{
    std::size_t operator()(const std::pair<int, int>& coord) const
    {
        return std::hash<int>()(coord.first ^ coord.second);
    }
};
} // namespace std

namespace pathfind
{
// note that instances of this type are assumed to be thread-local, therefore
// the type is not thread safe
class Map
{
    friend class Tile;

private:
    static constexpr int MaxStackedPolys = 128;
    static constexpr int MaxPathHops = 4096;

    BVH m_bvhLoader;

    bool m_hasADT[MeshSettings::Adts][MeshSettings::Adts];
    bool m_loadedADT[MeshSettings::Adts][MeshSettings::Adts];

    const std::filesystem::path m_dataPath;
    const std::string m_mapName;

    dtNavMesh m_navMesh;
    dtNavMeshQuery m_navQuery;
    dtQueryFilter m_queryFilter;

    // TODO: Does this need to be a pointer?
    std::unordered_map<std::pair<int, int>, std::unique_ptr<Tile>> m_tiles;

    // indexed by unique instance id.  this data is always loaded.  whenever a
    // tile using one of these instances is loaded, the corresponding model is
    // loaded also.  whenever all tiles referencing a model (possibly through
    // distinct instances) are unloaded, the model is unloaded.
    std::unordered_map<std::uint32_t, WmoInstance> m_staticWmos;
    std::unordered_map<std::uint32_t, DoodadInstance> m_staticDoodads;

    // indexed by GUID
    std::unordered_map<std::uint64_t, std::weak_ptr<WmoInstance>>
        m_temporaryWmos;
    std::unordered_map<std::uint64_t, std::weak_ptr<DoodadInstance>>
        m_temporaryDoodads;

    // map, by filename, of loaded models
    std::unordered_map<std::string, std::weak_ptr<WmoModel>> m_loadedWmoModels;
    std::unordered_map<std::string, std::weak_ptr<DoodadModel>>
        m_loadedDoodadModels;

    // ensures that the model for a particular WMO instance is loaded
    std::shared_ptr<WmoModel> LoadModelForWmoInstance(unsigned int instanceId);

    // ensures that the model for a particular doodad instance is loaded
    std::shared_ptr<DoodadModel>
    LoadModelForDoodadInstance(unsigned int instanceId);

    // ensure that the given WMO model is loaded
    std::shared_ptr<WmoModel> EnsureWmoModelLoaded(const std::string& mpq_path);

    // ensure that the given doodad model is loaded
    std::shared_ptr<DoodadModel>
    EnsureDoodadModelLoaded(const std::string& mpq_path);

    const Tile* GetTile(float x, float y) const;

    bool GetADTHeight(const Tile* tile, float x, float y, float& height,
                      unsigned int* zone = nullptr,
                      unsigned int* area = nullptr) const;

    // find a more precise z value at or below the given hint.  the purpose of
    // this is to refine the z value for the final hop on a path, and should not
    // be exposed to clients, as it assumes that the hint is within the Recast
    // DetailSampleMaxError, and users cannot be trusted to honor this
    // restriction.  also, they shouldn't have to, since we want to take care of
    // it for them
    bool FindPreciseZ(const Tile* tile, float x, float y, float zHint,
                      bool includeAdt, float& result) const;

    bool RayCast(math::Ray& ray, bool doodads) const;
    bool RayCast(math::Ray& ray, const std::vector<const Tile*>& tiles,
                 bool doodads, unsigned int* zone = nullptr,
                 unsigned int* area = nullptr) const;

    // TODO: need mechanism to cleanup expired weak pointers saved in the
    // containers of this class

public:
    Map() = delete;
    Map(const Map&) = delete;
    Map(const std::string& dataPath, const std::string& mapName);

    bool HasADT(int x, int y) const;
    bool IsADTLoaded(int x, int y) const;
    bool LoadADT(int x, int y);
    void UnloadADT(int x, int y);
    int LoadAllADTs();

    // rotation specified in radians rotated around Z axis
    void AddGameObject(std::uint64_t guid, unsigned int displayId,
                       const math::Vertex& position, float orientation,
                       int doodadSet = -1);
    void AddGameObject(std::uint64_t guid, unsigned int displayId,
                       const math::Vertex& position,
                       const math::Quaternion& rotation, int doodadSet = -1);
    void AddGameObject(std::uint64_t guid, unsigned int displayId,
                       const math::Vertex& position,
                       const math::Matrix& rotation, int doodadSet = -1);

    std::shared_ptr<Model> GetOrLoadModelByDisplayId(unsigned int displayId);

    bool FindPath(const math::Vertex& start, const math::Vertex& end,
                  std::vector<math::Vertex>& output,
                  bool allowPartial = false) const;

    // for finding height(s) at a given (x, y), there are two scenarios:
    // 1: we want to find exactly one z for a given path which has this (x, y)
    // as a hop.  in this case, there should only be one correct value,
    //    and it is the value that would be achieved by walking either from the
    //    previous hop to this one, or from this hop to the next one.
    // 2: for a given (x, y), we want to find all possible z values
    //
    // NOTE: if your usage is outside of both of these scenarios, you are
    // probably doing something wrong
    float FindHeight(const math::Vertex& source,
                     const math::Vertex& target) const; // scenario one
    bool FindHeights(float x, float y,
                     std::vector<float>& output) const; // scenario two

    bool ZoneAndArea(const math::Vertex& position, unsigned int& zone,
                     unsigned int& area) const;

    // Returns true when there is line of sight from the start position to
    // the stop position.  The intended use of this is for spells and NPC
    // aggro, so doodads and temporary obstacles will be ignored.
    bool LineOfSight(const math::Vertex& start, const math::Vertex& stop) const;

    const dtNavMesh& GetNavMesh() const { return m_navMesh; }
    const dtNavMeshQuery& GetNavMeshQuery() const { return m_navQuery; }
};
} // namespace pathfind
