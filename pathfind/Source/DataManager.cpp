#include "parser.hpp"
#include "Output/Continent.hpp"
#include "DataManager.hpp"

#include <string>

namespace pathfind
{
namespace build
{

DataManager::DataManager(const std::string &continentName)
{
    parser::Parser::Initialize();

    m_continent.reset(new parser::Continent(continentName));
}

}
}