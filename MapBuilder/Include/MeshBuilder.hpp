#pragma once

#include "parser/Include/Map/Map.hpp"
#include "parser/Include/Wmo/Wmo.hpp"

#include "utility/Include/LinearAlgebra.hpp"

#include <vector>
#include <string>
#include <mutex>
#include <unordered_set>

class MeshBuilder
{
    private:
        std::unique_ptr<parser::Map> m_map;
        const std::string m_outputPath;

        std::vector<std::pair<int, int>> m_pendingAdts;
        int m_adtReferences[64][64];

        std::unordered_set<std::string> m_bvhWmos;
        std::unordered_set<std::string> m_bvhDoodads;

        mutable std::mutex m_mutex;

        void AddReference(int adtX, int adtY);

        void SerializeWmo(const parser::Wmo *wmo);
        void SerializeDoodad(const parser::Doodad *doodad);

    public:
        MeshBuilder(const std::string &dataPath, const std::string &outputPath, const std::string &mapName);

        int AdtCount() const;

        void SingleAdt(int adtX, int adtY);
        bool GetNextAdt(int &adtX, int &adtY);
        
        bool IsGlobalWMO() const;
        void RemoveReference(int adtX, int adtY);

        bool GenerateAndSaveGlobalWMO();
        bool GenerateAndSaveTile(int adtX, int adtY);

        bool GenerateAndSaveGSet();
        bool GenerateAndSaveGSet(int adtX, int adtY);

        void SaveMap() const;
};
