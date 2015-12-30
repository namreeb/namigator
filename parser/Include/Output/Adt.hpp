#pragma once

#include <string>
#include <list>
#include <vector>
#include <set>
#include <memory>

#include "Output/Wmo.hpp"
#include "Output/Doodad.hpp"

namespace parser
{
    struct AdtChunk
    {
        bool m_holeMap[8][8];

        std::vector<Vertex> m_terrainVertices;
        std::vector<int> m_terrainIndices;

        std::vector<Vector3> m_surfaceNormals;

        std::vector<Vertex> m_liquidVertices;
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
            Continent *m_continent;

        public:
            const int X;
            const int Y;

            const float MaxX;
            const float MinX;

            const float MaxY;
            const float MinY;

            Adt(Continent *continent, int x, int y);

            void WriteObjFile() const;

            const AdtChunk *GetChunk(int chunkX, int chunkY) const;
    };
}