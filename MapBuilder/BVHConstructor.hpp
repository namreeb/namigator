#pragma once

#include <mutex>
#include <string>
#include <cstdint>
#include <experimental/filesystem>
#include <unordered_map>
#include <atomic>

namespace fs = std::experimental::filesystem;

class BVHConstructor
{
private:
    const fs::path m_outputPath;
    // this will track all serialized wmos and doodads and map to
    // their .bvh path
    std::unordered_map<std::string, std::string> m_files;

    // this will track those serialized wmos and doodads which
    // can be used as temporary obstacles because they have an id
    // that can be referenced later.  maps to .bvh path.
    std::unordered_map<int, std::string> m_temporaryObstacles;

    std::atomic_bool m_shutdown;

    std::mutex m_mutex;

    // assumes the mutex has already been locked
    fs::path InternalAddFile(const fs::path& mpq_path);

public:
    BVHConstructor(const fs::path& outputPath);
    ~BVHConstructor();

    fs::path AddFile(const fs::path& mpq_path);
    fs::path AddTemporaryObstacle(std::uint32_t id, const fs::path& mpq_path);

    void Shutdown();
};