#pragma once

#include "Tile.hpp"
#include "Model.hpp"

#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/Ray.hpp"

#include "Detour/Include/DetourNavMesh.h"
#include "Detour/Include/DetourNavMeshQuery.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace std
{
template <>
struct hash<std::pair<int,int>>
{
    std::size_t operator()(const std::pair<int, int>& coord) const
    {
        return std::hash<int>()(coord.first ^ coord.second);
    }
};
}

namespace pathfind
{
// note that instances of this type are assumed to be thread-local, therefore the type is not thread safe
class Map
{
    friend class Tile;

    private:
        static constexpr int MaxPathHops = 128;
        static constexpr unsigned int GlobalWmoId = 0xFFFFFFFF;

        const std::string m_dataPath;
        const std::string m_mapName;

        dtNavMesh m_navMesh;
        dtNavMeshQuery m_navQuery;
        dtQueryFilter m_queryFilter;
        
        std::unordered_map<std::pair<int, int>, std::unique_ptr<Tile>> m_tiles;

        // indexed by display id
        std::unordered_map<unsigned int, std::string> m_temporaryObstaclePaths;     // same for all maps, but no global object to store it in

        // indexed by unique instance id.  this data is always loaded.  whenever a tile using one of these instances is loaded, the corresponding
        // model is loaded also.  whenever all tiles referencing a model (possibly through distinct instances) are unloaded, the model is unloaded.
        std::unordered_map<std::uint32_t, WmoInstance> m_staticWmos;
        std::unordered_map<std::uint32_t, DoodadInstance> m_staticDoodads;

        // indexed by GUID
        std::unordered_map<std::uint64_t, std::weak_ptr<WmoInstance>> m_temporaryWmos;
        std::unordered_map<std::uint64_t, std::weak_ptr<DoodadInstance>> m_temporaryDoodads;

        // map, by filename, of loaded models
        std::unordered_map<std::string, std::weak_ptr<WmoModel>> m_loadedWmoModels;
        std::unordered_map<std::string, std::weak_ptr<DoodadModel>> m_loadedDoodadModels;

        // ensures that the model for a particular WMO instance is loaded
        std::shared_ptr<WmoModel> LoadModelForWmoInstance(unsigned int instanceId);

        // ensures that the model for a particular doodad instance is loaded
        std::shared_ptr<DoodadModel> LoadModelForDoodadInstance(unsigned int instanceId);

        // ensure that the given WMO model is loaded
        std::shared_ptr<WmoModel> EnsureWmoModelLoaded(const std::string &filename);

        // ensure that the given doodad model is loaded
        std::shared_ptr<DoodadModel> EnsureDoodadModelLoaded(const std::string &filename);

        // TODO: need mechanism to cleanup expired weak pointers saved in the containers of this class

    public:
        Map(const std::string &dataPath, const std::string &mapName);

        bool LoadADT(int x, int y);
        void UnloadADT(int x, int y);

        // rotation specified in radians rotated around Z axis
        void AddGameObject(std::uint64_t guid, unsigned int displayId, const utility::Vertex &position, float orientation, int doodadSet = -1);
        void AddGameObject(std::uint64_t guid, unsigned int displayId, const utility::Vertex &position, const utility::Quaternion &rotation, int doodadSet = -1);
        void AddGameObject(std::uint64_t guid, unsigned int displayId, const utility::Vertex &position, const utility::Matrix &rotation, int doodadSet = -1);

        bool FindPath(const utility::Vertex &start, const utility::Vertex &end, std::vector<utility::Vertex> &output, bool allowPartial = false) const;
        bool FindHeights(const utility::Vertex &position, std::vector<float> &output) const;
        bool FindHeights(float x, float y, std::vector<float> &output) const;
        bool RayCast(utility::Ray &ray) const;

        const dtNavMesh &GetNavMesh() const { return m_navMesh; }
        const dtNavMeshQuery &GetNavMeshQuery() const { return m_navQuery; }
};
}