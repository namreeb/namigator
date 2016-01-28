#include "MpqManager.hpp"
#include "parser.hpp"

namespace parser
{
void Parser::Initialize()
{
    MpqManager::Initialize();
}

void Parser::Initialize(const char *path)
{
    MpqManager::Initialize(path);
}
}