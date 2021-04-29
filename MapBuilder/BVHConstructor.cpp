#include "BVHConstructor.hpp"

#include "utility/BinaryStream.hpp"
#include "utility/Exception.hpp"
#include "utility/PicoSHA2/picosha2.h"

#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

BVHConstructor::BVHConstructor(const fs::path& outputPath)
    : m_outputPath(outputPath), m_shutdown(false)
{
    auto const index_file = m_outputPath / "BVH" / "bvh.idx";
    if (!fs::is_regular_file(index_file))
        return;

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

BVHConstructor::~BVHConstructor()
{
    if (!m_shutdown)
        Shutdown();
}

fs::path BVHConstructor::InternalAddFile(const fs::path& mpq_path)
{
    auto const it = m_files.find(mpq_path.string());

    if (it != m_files.end())
        return it->second;

    auto const extension = mpq_path.extension().string();

    std::string kind;
    if (extension[1] == 'm' || extension[1] == 'M')
        kind = "Doodad";
    else if (extension[1] == 'w' || extension[1] == 'W')
        kind = "WMO";
    else
        THROW("Unrecognized extension");

    // compute sha256 checksum of path in mpq to give a unique name without
    // symbols
    std::vector<unsigned char> hash(picosha2::k_digest_size);

    auto const mpq_path_str = mpq_path.string();
    std::vector<char> filename_vect;
    std::copy(mpq_path_str.begin(), mpq_path_str.end(),
              std::back_inserter(filename_vect));

    picosha2::hash256(filename_vect, hash);

    std::stringstream str;
    for (auto const c : hash)
        str << std::hex << std::setw(2) << std::setfill('0') << (int)c;

    return m_files[mpq_path.string()] = kind + "_" + str.str() + ".bvh";
}

fs::path BVHConstructor::AddFile(const fs::path& mpq_path)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    return m_outputPath / "BVH" / InternalAddFile(mpq_path);
}

fs::path BVHConstructor::AddTemporaryObstacle(std::uint32_t id,
                                              const fs::path& mpq_path)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    return m_outputPath / "BVH" /
           (m_temporaryObstacles[id] = InternalAddFile(mpq_path).string());
}

void BVHConstructor::Shutdown()
{
    if (m_shutdown)
        return;

    std::lock_guard<std::mutex> guard(m_mutex);

    utility::BinaryStream out;

    // sort the final results because why not?
    std::vector<std::pair<std::string, std::string>> serialized(m_files.begin(),
                                                                m_files.end());
    m_files.clear();
    std::sort(serialized.begin(), serialized.end());

    out << static_cast<std::uint32_t>(serialized.size()); // entry count

    for (auto const& entry : serialized)
        out << static_cast<std::uint32_t>(entry.first.length()) << entry.first
            << static_cast<std::uint32_t>(entry.second.length())
            << entry.second;

    std::vector<std::pair<std::uint32_t, std::string>> obstacles(
        m_temporaryObstacles.begin(), m_temporaryObstacles.end());
    m_temporaryObstacles.clear();
    std::sort(obstacles.begin(), obstacles.end());

    out << static_cast<std::uint32_t>(obstacles.size());
    for (auto const& entry : obstacles)
        out << entry.first << static_cast<std::uint32_t>(entry.second.length())
            << entry.second;

    std::ofstream o(m_outputPath / "BVH" / "bvh.idx",
                    std::ofstream::binary | std::ofstream::trunc);
    o << out;

    m_shutdown = true;
}