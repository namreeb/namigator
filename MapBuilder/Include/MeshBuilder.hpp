#pragma once

#include "parser/Include/Map/Map.hpp"

#include "utility/Include/LinearAlgebra.hpp"

#include <vector>
#include <string>
#include <mutex>
#include <set>

#ifdef _DEBUG
#include <ostream>
#endif

class MeshBuilder
{
    private:
        std::unique_ptr<parser::Map> m_map;
        const std::string m_outputPath;

        std::vector<std::pair<int, int>> m_pendingAdts;
        int m_adtReferences[64][64];

        std::set<std::string> m_bvhWmos;
        std::set<std::string> m_bvhDoodads;

        mutable std::mutex m_mutex;

        void AddReference(int adtX, int adtY);

    public:
        MeshBuilder(const std::string &dataPath, const std::string &outputPath, const std::string &mapName);

        int AdtCount() const;

        void SingleAdt(int adtX, int adtY);
        bool GetNextAdt(int &adtX, int &adtY);
        
        bool IsGlobalWMO() const;
        void RemoveReference(int adtX, int adtY);

        bool GenerateAndSaveGlobalWMO();
        bool GenerateAndSaveTile(int adtX, int adtY);

#ifdef _DEBUG
        std::string AdtMap() const;
        std::string AdtReferencesMap() const;
        std::string WmoMap() const;

        void WriteMemoryUsage(std::ostream &stream) const;
#endif
};
