#pragma once

#include "parser/Include/Map/Map.hpp"
#include "parser/Include/Wmo/Wmo.hpp"

#include "utility/Include/LinearAlgebra.hpp"
#include "RecastDetourBuild/Include/Common.hpp"

#include <vector>
#include <string>
#include <mutex>
#include <unordered_set>

class MeshBuilder
{
    private:
        std::unique_ptr<parser::Map> m_map;
        const std::string m_outputPath;

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

    public:
        MeshBuilder(const std::string &dataPath, const std::string &outputPath, const std::string &mapName, int logLevel);
        MeshBuilder(const std::string &dataPath, const std::string &outputPath, const std::string &mapName, int logLevel, int adtX, int adtY);

        int TotalTiles() const;

        bool GetNextTile(int &tileX, int &tileY);
        
        bool IsGlobalWMO() const;

        bool GenerateAndSaveGlobalWMO();
        bool GenerateAndSaveTile(int tileX, int tileY);

        bool GenerateAndSaveGSet() const;
        //bool GenerateAndSaveGSet(int tileX, int tileY);

        void SaveMap() const;

        float PercentComplete() const;
};