#pragma once

#include "parser/Include/Output/Continent.hpp"
#include "utility/Include/LinearAlgebra.hpp"

#include <vector>
#include <string>

namespace pathfind
{
namespace build
{
class MeshBuilder
{
    private:
        std::unique_ptr<parser::Continent> m_continent;
        const std::string m_outputPath;

    public:
        MeshBuilder(const std::string &dataPath, const std::string &outputPath, std::string &continentName);

        bool IsGlobalWMO() const;

        bool GenerateAndSaveGlobalWMO();
        bool GenerateAndSaveTile(int adtX, int adtY);
};
}
}