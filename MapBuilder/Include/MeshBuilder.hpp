#pragma once

#include "parser/Include/Output/Continent.hpp"
#include "utility/Include/LinearAlgebra.hpp"

#include <queue>
#include <string>
#include <mutex>

class MeshBuilder
{
    private:
        std::unique_ptr<parser::Continent> m_continent;
        const std::string m_outputPath;

        std::mutex m_mutex;
        std::queue<std::pair<int, int>> m_adts;
        int m_adtReferences[64][64];

        void AddReference(int adtX, int adtY);

    public:
        MeshBuilder(const std::string &dataPath, const std::string &outputPath, std::string &continentName);

        void SingleAdt(int adtX, int adtY);
        bool GetNextAdt(int &adtX, int &adtY);
        
        bool IsGlobalWMO() const;
        void RemoveReference(int adtX, int adtY);

        bool GenerateAndSaveGlobalWMO();
        bool GenerateAndSaveTile(int adtX, int adtY);
};
