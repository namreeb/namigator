#include "BVH.hpp"

#include "utility/BinaryStream.hpp"
#include "utility/Exception.hpp"

namespace pathfind
{
BVH::BVH(const fs::path& path) : m_dataPath(path)
{
    auto const index_file = m_dataPath / "BVH" / "bvh.idx";

    if (!fs::is_regular_file(index_file))
        THROW(Result::BVH_INDEX_FILE_NOT_FOUND);

    utility::BinaryStream index(index_file);

    std::uint32_t num_bvh, num_obstacles;
    index >> num_bvh;

    for (auto i = 0u; i < num_bvh; ++i)
    {
        std::uint32_t length;
        index >> length;

        auto const mpq_path = index.ReadString(length);

        index >> length;

        auto const bvh_path = index.ReadString(length);

        m_files[mpq_path] = bvh_path;
    }

    index >> num_obstacles;

    for (auto i = 0u; i < num_obstacles; ++i)
    {
        std::uint32_t entry, length;

        index >> entry >> length;

        m_temporaryObstacles[entry] = index.ReadString(length);
    }
}

std::string BVH::GetBVHPath(const std::string& mpq_path) const
{
    auto const result = m_files.find(mpq_path);

    if (result == m_files.end())
        THROW(Result::REQUESTED_BVH_NOT_FOUND);

    return (m_dataPath / "BVH" / result->second).string();
}

std::string BVH::GetBVHPath(std::uint32_t entry) const
{
    auto const result = m_temporaryObstacles.find(entry);

    if (result == m_temporaryObstacles.end())
        THROW(Result::REQUESTED_BVH_NOT_FOUND);

    return (m_dataPath / "BVH" / result->second).string();
}

std::string BVH::GetMPQPath(std::uint32_t entry) const
{
    return "";
}
} // namespace pathfind