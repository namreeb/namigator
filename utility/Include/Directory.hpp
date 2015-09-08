#pragma once

#include <string>

namespace utility
{
    class Directory
    {
        public:
            static bool Exists(const std::string &path);
            static bool Exists(const char *path);

            static void Create(const std::string &path);
            static void Create(const char *path);
            
            static std::string Current();
    };
}