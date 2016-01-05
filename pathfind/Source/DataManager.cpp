#include "parser/Include/parser.hpp"
#include "parser/Include/Output/Continent.hpp"
#include "DataManager.hpp"

#include <string>

namespace pathfind
{
namespace build
{
DataManager::DataManager(const std::string &dataPath, const std::string &outputPath, std::string &continentName) : m_outputPath(outputPath)
{
    parser::Parser::Initialize(dataPath.c_str());

    m_continent.reset(new parser::Continent(continentName));
}
}
}