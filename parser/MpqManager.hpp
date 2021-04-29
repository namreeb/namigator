#pragma once

#include "utility/BinaryStream.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace parser
{
class MpqManager
{
private:
    using HANDLE = void*;

    std::vector<HANDLE> MpqHandles;
    std::unordered_map<std::string, unsigned int> Maps;
    std::unordered_map<std::uint32_t, std::uint32_t> AreaToZone;

    void LoadMpq(const std::string& filePath);

public:
    void Initialize();
    void Initialize(const std::string& wowDir);

    bool FileExists(const std::string& file) const;
    utility::BinaryStream* OpenFile(const std::string& file);

    unsigned int GetMapId(const std::string& name) const;
    unsigned int GetZoneId(unsigned int areaId) const;
};

extern thread_local MpqManager sMpqManager;
}; // namespace parser