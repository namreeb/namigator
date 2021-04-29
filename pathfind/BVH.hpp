#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace fs = std::filesystem;

namespace pathfind
{
class BVH
{
private:
    const fs::path m_dataPath;

    // map mpq path to .bvh path
    std::unordered_map<std::string, std::string> m_files;

    // map gameobject display id to .bvh path
    std::unordered_map<int, std::string> m_temporaryObstacles;

public:
    BVH(const fs::path& path);

    std::string GetBVHPath(const std::string& mpq_path) const;
    std::string GetBVHPath(std::uint32_t entry) const;

    std::string GetMPQPath(std::uint32_t entry) const;
};
} // namespace pathfind