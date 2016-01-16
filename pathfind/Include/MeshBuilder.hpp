#pragma once

#include "parser/Include/Output/Continent.hpp"
#include "utility/Include/LinearAlgebra.hpp"

#include <vector>
#include <string>
#include <mutex>

namespace pathfind
{
namespace build
{
class MeshBuilder
{
    private:
        std::unique_ptr<parser::Continent> m_continent;
        const std::string m_outputPath;

        std::mutex m_mutex;
        int m_adtReferences[64][64];

    public:
        MeshBuilder(const std::string &dataPath, const std::string &outputPath, std::string &continentName);

        void BuildWorkList(std::vector<std::pair<int, int>> &tiles) const;
        bool IsGlobalWMO() const;

        void AddReference(int adtX, int adtY);
        void RemoveReference(int adtX, int adtY);

        bool GenerateAndSaveGlobalWMO();
        bool GenerateAndSaveTile(int adtX, int adtY);
};
}
}