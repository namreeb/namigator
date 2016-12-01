#pragma once

#include "parser/Include/Map/Map.hpp"
#include "parser/Include/Wmo/Wmo.hpp"

#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/BinaryStream.hpp"
#include "RecastDetourBuild/Include/Common.hpp"

#include <vector>
#include <string>
#include <mutex>
#include <unordered_set>
#include <map>
#include <cstdint>

namespace meshfiles
{
    class File
    {
        protected:
            // serialized heightfield and finalized mesh data, mapped by tile id
            std::map<std::pair<std::int32_t, std::int32_t>, utility::BinaryStream> m_tiles;

            mutable std::mutex m_mutex;

            // this function assumes that the mutex has already been locked
            void AddTile(int x, int y, utility::BinaryStream &heightfield, const utility::BinaryStream &mesh);

        public:
            virtual ~File() = default;

            virtual void Serialize(const std::string &filename) const = 0;
    };

    class ADT : File
    {
        private:
            const int m_x;
            const int m_y;

            // serialized data for WMOs and doodad IDs, mapped by tile id within the ADT
            std::map<std::pair<int, int>, utility::BinaryStream> m_wmosAndDoodadIds;

        public:
            ADT(int x, int y) : m_x(x), m_y(y) {}

            virtual ~ADT() = default;

            // these x and y arguments refer to the tile x and y
            void AddTile(int x, int y, utility::BinaryStream &wmosAndDoodads, utility::BinaryStream &heightField, utility::BinaryStream &mesh);

            bool IsComplete() const { return m_tiles.size() == (MeshSettings::TilesPerADT*MeshSettings::TilesPerADT); }
            void Serialize(const std::string &filename) const override;
    };

    class GlobalWMO : File
    {
        public:
            virtual ~GlobalWMO() = default;

            void AddTile(int x, int y, utility::BinaryStream &heightField, utility::BinaryStream &mesh);

            void Serialize(const std::string &filename) const override;
    };
}

class MeshBuilder
{
    private:
        std::unique_ptr<parser::Map> m_map;
        const std::string m_outputPath;

        std::map<std::pair<int, int>, std::unique_ptr<meshfiles::ADT>> m_adtsInProgress;
        std::unique_ptr<meshfiles::GlobalWMO> m_globalWMO;

        std::vector<std::pair<int, int>> m_pendingTiles;
        std::vector<int> m_chunkReferences; // this is a fixed size, but it is so big that it can single-handedly overflow the stack

        std::vector<utility::Vertex> m_globalWMOVertices;
        std::vector<int> m_globalWMOIndices;

        std::vector<utility::Vertex> m_globalWMOLiquidVertices;
        std::vector<int> m_globalWMOLiquidIndices;

        std::vector<utility::Vertex> m_globalWMODoodadVertices;
        std::vector<int> m_globalWMODoodadIndices;

        std::unordered_set<std::string> m_bvhWmos;
        std::unordered_set<std::string> m_bvhDoodads;

        float m_minX, m_maxX, m_minY, m_maxY, m_minZ, m_maxZ;

        mutable std::mutex m_mutex;

        size_t m_totalTiles;
        size_t m_completedTiles;

        const int m_logLevel;

        void AddChunkReference(int chunkX, int chunkY);
        void RemoveChunkReference(int chunkX, int chunkY);

        void SerializeWmo(const parser::Wmo *wmo);
        void SerializeDoodad(const parser::Doodad *doodad);

        // these two functions assume ownership of the mutex
        meshfiles::ADT *GetInProgressADT(int x, int y);
        void RemoveADT(const meshfiles::ADT *adt);

    public:
        MeshBuilder(const std::string &outputPath, const std::string &mapName, int logLevel);
        MeshBuilder(const std::string &outputPath, const std::string &mapName, int logLevel, int adtX, int adtY);

        size_t CompletedTiles() const { return m_completedTiles; }

        bool GetNextTile(int &tileX, int &tileY);
        
        bool IsGlobalWMO() const;

        bool BuildAndSerializeWMOTile(int tileX, int tileY);
        bool BuildAndSerializeMapTile(int tileX, int tileY);

        void SaveMap() const;

        float PercentComplete() const;
};
