#include "MpqManager.hpp"
#include "parser.hpp"

#include <string>

namespace parser
{
void Parser::Initialize()
{
    MpqManager::Initialize();
}

void Parser::Initialize(const std::string &path)
{
    MpqManager::Initialize(path);
}
}