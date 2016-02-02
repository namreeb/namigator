#pragma once

#include "parser/Include/Wmo/WmoInstance.hpp"
#include "parser/Include/Doodad/DoodadInstance.hpp"

#include "utility/Include/BoundingBox.hpp"

#include <string>
#include <list>
#include <vector>
#include <unordered_set>
#include <memory>

namespace parser
{
struct AdtChunk
{
    bool m_holeMap[8][8];

    std::vector<utility::Vertex> m_terrainVertices;
    std::vector<int> m_terrainIndices;

    std::vector<utility::Vertex> m_liquidVertices;
    std::vector<int> m_liquidIndices;
};

class Map;

class Adt
{
    private:
        std::unique_ptr<AdtChunk> m_chunks[16][16];
        Map * const m_map;

    public:
        const int X;
        const int Y;

        utility::BoundingBox Bounds;

        std::unordered_set<unsigned int> WmoInstances;
        std::unordered_set<unsigned int> DoodadInstances;

        Adt(Map *map, int x, int y);
        const AdtChunk *GetChunk(int chunkX, int chunkY) const;
};
}