#pragma once

#include "Tile.hpp"
#include "Model.hpp"

#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/BoundingBox.hpp"
#include "utility/Include/AABBTree.hpp"
#include "utility/Include/Ray.hpp"

#include "Detour/Include/DetourNavMesh.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <list>

namespace pathfind
{
// note that instances of this type are assumed to be thread-local, therefore the type is not thread safe
class Map
{
    friend class Tile;

    private:
        const std::string m_dataPath;
        const std::string m_mapName;

        dtNavMesh m_navMesh;

        std::unique_ptr<Tile> m_tiles[64][64];
        std::shared_ptr<WmoModel> m_globalWmo;

        std::unordered_map<unsigned int, WmoInstance> m_wmoInstances;
        std::unordered_map<unsigned int, DoodadInstance> m_doodadInstances;

        std::unordered_map<std::string, std::weak_ptr<WmoModel>> m_loadedWmoModels;
        std::unordered_map<std::string, std::weak_ptr<DoodadModel>> m_loadedDoodadModels;

        std::shared_ptr<WmoModel> LoadWmoModel(unsigned int id);

        std::shared_ptr<DoodadModel> LoadDoodadModel(unsigned int id);
        std::shared_ptr<DoodadModel> LoadDoodadModel(const std::string &fileName);

    public:
        Map(const std::string &dataPath, const std::string &mapName);

        void LoadAllTiles();
        bool LoadTile(int x, int y);
        void UnloadTile(int x, int y);

        bool FindPath(const utility::Vertex &start, const utility::Vertex &stop, std::vector<utility::Vertex> &output) const;   // NOT IMPLEMENTED YET!
        bool FindHeights(const utility::Vertex &position, std::vector<float> &output) const;
        bool FindHeights(float x, float y, std::vector<float> &output) const;                                                   // NOT IMPLEMENTED YET!
        bool RayCast(const utility::Ray &ray, utility::Vertex &collision) const;                                                // NOT IMPLEMENTED YET!

        void GetTileGeometry(int x, int y, std::vector<utility::Vertex> &vertices, std::vector<int> &indices) const;
};
}