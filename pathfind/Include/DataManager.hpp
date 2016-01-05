#pragma once

#include "parser/Include/Output/Continent.hpp"

#include <string>
#include <memory>

namespace pathfind
{
namespace build
{
class DataManager
{
    friend class MeshBuilder;

    private:
        std::unique_ptr<parser::Continent> m_continent;
        const std::string m_outputPath;

    public:
        DataManager(const std::string &dataPath, const std::string &outputPath, std::string &continentName);
};
}
}