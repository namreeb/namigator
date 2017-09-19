#pragma once

#include <string>

namespace parser
{
class Parser
{
    public:
        static void Initialize();
        static void Initialize(const std::string &path);
};
}