#pragma once

#include "utility/BinaryStream.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

namespace parser
{
class MpqManager
{
private:
    using HANDLE = void*;

    bool Alpha;

    fs::path BasePath;

    std::unordered_map<std::string, HANDLE> MpqHandles;
    std::unordered_map<std::string, unsigned int> Maps;
    std::unordered_map<std::uint32_t, std::uint32_t> AreaToZone;

    void LoadMpq(const std::filesystem::path& filePath);

public:
    void Initialize();
    void Initialize(const fs::path& wowDir);

    bool FileExists(const std::string& file) const;
    std::unique_ptr<utility::BinaryStream> OpenFile(const std::string& file);

    unsigned int GetMapId(const std::string& name) const;
    unsigned int GetZoneId(unsigned int areaId) const;
};

extern thread_local MpqManager sMpqManager;
}; // namespace parser