#pragma once

#include "Tile.hpp"
#include "Model.hpp"

#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/Ray.hpp"

#include "recastnavigation/Detour/Include/DetourNavMesh.h"
#include "recastnavigation/Detour/Include/DetourNavMeshQuery.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <experimental/filesystem>

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
        static constexpr int MaxStackedPolys = 128;
        static constexpr int MaxPathHops = 128;
        static constexpr unsigned int GlobalWmoId = 0xFFFFFFFF;

        const std::experimental::filesystem::path m_dataPath;
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
        std::shared_ptr<WmoModel> EnsureWmoModelLoaded(const std::string &filename, bool isBvhFilename = false);

        // ensure that the given doodad model is loaded
        std::shared_ptr<DoodadModel> EnsureDoodadModelLoaded(const std::string &filename, bool isBvhFilename = false);

        // find a more precise z value at or below the given hint.  the purpose of this is to refine
        // the z value for the final hop on a path, and should not be exposed to clients, as it assumes
        // that the hint is within the Recast DetailSampleMaxError, and users cannot be trusted to
        // honor this restriction.  also, they shouldn't have to, since we want to take care of it for them
        float FindPreciseZ(float x, float y, float zHint) const;

        bool RayCast(utility::Ray &ray) const;
        bool RayCast(utility::Ray &ray, const std::vector<const Tile *> &tiles) const;

        // TODO: need mechanism to cleanup expired weak pointers saved in the containers of this class

    public:
        Map(const std::string &dataPath, const std::string &mapName);

        bool LoadADT(int x, int y);
        void UnloadADT(int x, int y);

        // rotation specified in radians rotated around Z axis
        void AddGameObject(std::uint64_t guid, unsigned int displayId, const utility::Vertex &position, float orientation, int doodadSet = -1);
        void AddGameObject(std::uint64_t guid, unsigned int displayId, const utility::Vertex &position, const utility::Quaternion &rotation, int doodadSet = -1);
        void AddGameObject(std::uint64_t guid, unsigned int displayId, const utility::Vertex &position, const utility::Matrix &rotation, int doodadSet = -1);

        std::shared_ptr<Model> GetOrLoadModelByDisplayId(unsigned int displayId);

        bool FindPath(const utility::Vertex &start, const utility::Vertex &end, std::vector<utility::Vertex> &output, bool allowPartial = false) const;

        // for finding height(s) at a given (x, y), there are two scenarios:
        // 1: we want to find exactly one z for a given path which has this (x, y) as a hop.  in this case, there should only be one correct value,
        //    and it is the value that would be achieved by walking either from the previous hop to this one, or from this hop to the next one. 
        // 2: for a given (x, y), we want to find all possible z values
        //
        // NOTE: if your usage is outside of both of these scenarios, you are probably doing something wrong
        float FindHeight(const utility::Vertex &source, const utility::Vertex &target) const;       // scenario one
        bool FindHeights(float x, float y, std::vector<float> &output) const;                       // scenario two

        const dtNavMesh &GetNavMesh() const { return m_navMesh; }
        const dtNavMeshQuery &GetNavMeshQuery() const { return m_navQuery; }
};
}