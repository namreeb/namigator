#pragma once

#include "utility/BinaryStream.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace parser
{
class MpqManager
{
    private:
        using HANDLE = void *;

        static std::mutex _mutex;

        static std::vector<HANDLE> MpqHandles;
        static std::unordered_map<std::string, unsigned int> Maps;

        static void LoadMpq(const std::string &filePath);

    public:
        static void Initialize();
        static void Initialize(const std::string &wowDir);

        static utility::BinaryStream *OpenFile(const std::string &file);

        static unsigned int GetMapId(const std::string &name);
};
};