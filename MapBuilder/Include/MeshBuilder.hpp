#pragma once

#include "parser/Include/Output/Continent.hpp"
#include "utility/Include/LinearAlgebra.hpp"

#include <vector>
#include <string>
#include <mutex>

#ifdef _DEBUG
#include <ostream>
#endif

class MeshBuilder
{
    private:
        std::unique_ptr<parser::Continent> m_continent;
        const std::string m_outputPath;

        mutable std::mutex m_mutex;
        std::vector<std::pair<int, int>> m_pendingAdts;
        int m_adtReferences[64][64];

        void AddReference(int adtX, int adtY);

    public:
        MeshBuilder(const std::string &dataPath, const std::string &outputPath, const std::string &continentName);

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
