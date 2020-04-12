#pragma once

#include "utility/BinaryStream.hpp"

#include <string>
#include <vector>
#include <unordered_map>

namespace parser
{
class MpqManager
{
    private:
        using HANDLE = void *;

        std::vector<HANDLE> MpqHandles;
        std::unordered_map<std::string, unsigned int> Maps;

        void LoadMpq(const std::string &filePath);

    public:
        void Initialize();
        void Initialize(const std::string &wowDir);

        utility::BinaryStream *OpenFile(const std::string &file);

        unsigned int GetMapId(const std::string &name);
};

extern thread_local MpqManager sMpqManager;
};