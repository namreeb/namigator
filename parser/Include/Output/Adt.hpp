#pragma once

#include "parser/Include/Output/Wmo.hpp"
#include "parser/Include/Output/Doodad.hpp"

#include "utility/Include/BoundingBox.hpp"

#include <string>
#include <list>
#include <vector>
#include <set>
#include <memory>

namespace parser
{
struct AdtChunk
{
    bool m_holeMap[8][8];

    std::vector<utility::Vertex> m_terrainVertices;
    std::vector<int> m_terrainIndices;

    std::vector<utility::Vector3> m_surfaceNormals;

    std::vector<utility::Vertex> m_liquidVertices;
    std::vector<int> m_liquidIndices;

    // set of unique wmo identifiers present in this chunk
    std::set<unsigned int> m_wmos;

    // set of unique doodad identifiers present in this chunk
    std::set<unsigned int> m_doodads;
};

class Continent;

class Adt
{
    private:
        static inline bool IsRendered(unsigned char mask[], int x, int y);

        std::unique_ptr<AdtChunk> m_chunks[16][16];
        Continent * const m_continent;

    public:
        const int X;
        const int Y;

        utility::BoundingBox Bounds;

        Adt(Continent *continent, int x, int y);

#ifdef _DEBUG
        void WriteObjFile() const;
        size_t GetWmoCount() const;
#endif

        const AdtChunk *GetChunk(int chunkX, int chunkY) const;
};
}