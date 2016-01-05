#pragma once

#include <string>

namespace utility
{
class Directory
{
    public:
    static bool Exists(const std::string &path);
    static void Create(const std::string &path);

    static std::string Current();
};
}