#pragma once

#include "parser/Include/Map/Map.hpp"
#include "parser/Include/Wmo/Wmo.hpp"

#include "utility/Include/LinearAlgebra.hpp"
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
            std::map<std::pair<int, int>, std::vector<std::uint8_t>> m_tiles;

            mutable std::mutex m_mutex;

            // this function assumes that the mutex has already been locked
            void AddTile(int x, int y, const std::vector<std::uint8_t> &data);

        public:
            virtual ~File() = default;

            virtual bool IsComplete() const = 0;
            virtual void Serialize(const std::string &filename) const = 0;
    };

    class ADT : protected File
    {
        private:
            const int m_x;
            const int m_y;

            std::map<std::pair<int, int>, std::vector<std::uint32_t>> m_wmoIds;
            std::map<std::pair<int, int>, std::vector<std::uint32_t>> m_doodadIds;

        public:
            ADT(int x, int y);

            virtual ~ADT() = default;

            // these x and y arguments refer to the tile x and y
            void AddTile(int x, int y, const std::unordered_set<std::uint32_t> &wmoIds, const std::unordered_set<std::uint32_t> &doodadIds, const std::vector<std::uint8_t> &tileData);

            bool IsComplete() const override;
            void Serialize(const std::string &filename) const override;
    };

    class GlobalWMO : protected File
    {
        public:
            virtual ~GlobalWMO() = default;
    };
}

class MeshBuilder
{
    private:
        std::unique_ptr<parser::Map> m_map;
        const std::string m_outputPath;

        std::map<std::pair<int, int>, std::unique_ptr<meshfiles::ADT>> m_adtsInProgress;

        std::vector<std::pair<int, int>> m_pendingTiles;
        std::vector<int> m_chunkReferences; // this is a fixed size, but it is so big it can single-handedly overflow the stack

        std::unordered_set<std::string> m_bvhWmos;
        std::unordered_set<std::string> m_bvhDoodads;

        mutable std::mutex m_mutex;

        size_t m_startingTiles;
        size_t m_completedTiles;

        const int m_logLevel;

        void AddChunkReference(int chunkX, int chunkY);
        void RemoveChunkReference(int chunkX, int chunkY);

        void SerializeWmo(const parser::Wmo *wmo);
        void SerializeDoodad(const parser::Doodad *doodad);

        meshfiles::ADT *GetInProgressADT(int x, int y);
        void RemoveADT(const meshfiles::ADT *adt);

    public:
        MeshBuilder(const std::string &dataPath, const std::string &outputPath, const std::string &mapName, int logLevel);
        MeshBuilder(const std::string &dataPath, const std::string &outputPath, const std::string &mapName, int logLevel, int adtX, int adtY);

        size_t TotalTiles() const;

        bool GetNextTile(int &tileX, int &tileY);
        
        bool IsGlobalWMO() const;

        bool GenerateAndSaveGlobalWMO();
        bool GenerateAndSaveTile(int tileX, int tileY);

        bool GenerateAndSaveGSet() const;
        //bool GenerateAndSaveGSet(int tileX, int tileY);

        void SaveMap() const;

        float PercentComplete() const;
};
