#include "GameObjectBVHBuilder.hpp"
#include "MeshBuilder.hpp"

#include "parser/Doodad/Doodad.hpp"
#include "parser/Wmo/Wmo.hpp"
#include "parser/DBC.hpp"

#include "utility/AABBTree.hpp"
#include "utility/Exception.hpp"
#include "utility/BinaryStream.hpp"

#include <string>
#include <unordered_map>
#include <cstdint>
#include <thread>
#include <mutex>
#include <iostream>
#include <sstream>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

namespace
{
fs::path BuildAbsoluteFilename(const fs::path &output_path, const fs::path &in, std::uint32_t id)
{
    auto const extension = in.extension().string();

    std::string kind;
    if (extension[1] == 'm' || extension[1] == 'M')
        kind = "Doodad";
    else if (extension[1] == 'w' || extension[1] == 'W')
        kind = "WMO";
    else
        THROW("Unrecognized extension");

    // files are based on id rather than their path to make handling those filenames easier on Linux
    return (output_path / "BVH" / (kind + "_" + std::to_string(id))).replace_extension("bvh");
}
}
GameObjectBVHBuilder::GameObjectBVHBuilder(const std::string& outputPath, size_t workers)
    : m_outputPath(outputPath), m_workers(workers), m_shutdownRequested(false)
{
    const parser::DBC displayInfo("DBFilesClient\\GameObjectDisplayInfo.dbc");

    for (auto i = 0u; i < displayInfo.RecordCount(); ++i)
    {
        auto const row = displayInfo.GetField(i, 0);
        auto const path = displayInfo.GetStringField(i, 1);

        if (!path.length())
            continue;

        auto const output = BuildAbsoluteFilename(m_outputPath, path, row);

        // mark existing files as already serialized so that they are included in the index file
        if (fs::exists(output))
        {
            m_serialized[row] = fs::path(output).filename().string();
            continue;
        }

        auto const extension = fs::path(path).extension().string();

        if (extension[1] == 'm' || extension[1] == 'M')
            m_doodads[row] = path;
        else if (extension[1] == 'w' || extension[1] == 'W')
            m_wmos[row] = path;
        else
            THROW("Unrecognized extension");
    }
}

GameObjectBVHBuilder::~GameObjectBVHBuilder()
{
    Shutdown();
}

void GameObjectBVHBuilder::Begin()
{
    // if zero threads are requested, execute synchronously
    if (!m_workers)
        Work();
    else
        for (auto i = 0u; i < m_workers; ++i)
            m_threads.push_back(std::thread(&GameObjectBVHBuilder::Work, this));
}

void GameObjectBVHBuilder::Shutdown()
{
    m_shutdownRequested = true;
    for (auto &thread : m_threads)
        thread.join();
    
    m_threads.clear();
}

void GameObjectBVHBuilder::WriteIndexFile() const
{
    std::lock_guard<std::mutex> guard(m_mutex);

    // sort the final results because why not?
    std::vector<std::pair<std::uint32_t, std::string>> serialized(m_serialized.begin(), m_serialized.end());
    std::sort(serialized.begin(), serialized.end());

    utility::BinaryStream out;

    out << static_cast<std::uint32_t>(serialized.size());   // entry count

    for (auto const &entry : serialized)
        out << entry.first << static_cast<std::uint32_t>(entry.second.length()) << entry.second;

    std::ofstream o(m_outputPath / "BVH" / "bvh.idx", std::ofstream::binary | std::ofstream::trunc);
    o << out;
}

void GameObjectBVHBuilder::Work()
{
    while (!m_shutdownRequested)
    {
        std::uint32_t entry;
        std::string filename;
        bool isDoodad;

        {
            std::lock_guard<std::mutex> guard(m_mutex);

            if (m_doodads.empty() && m_wmos.empty())
                break;

            // the following logic is probably more clever than it needs to be

            // if there is only one choice, choose it
            if (m_doodads.empty())
            {
                auto const i = m_wmos.begin();
                entry = i->first;
                filename = i->second;
                m_wmos.erase(i);
                isDoodad = false;
            }
            else if (m_wmos.empty())
            {
                auto const i = m_doodads.begin();
                entry = i->first;
                filename = i->second;
                m_doodads.erase(i);
                isDoodad = true;
            }
            // else, choose one at random
            else if (!(rand() % 2))
            {
                auto const i = m_wmos.begin();
                entry = i->first;
                filename = i->second;
                m_wmos.erase(i);
                isDoodad = false;
            }
            else
            {
                auto const i = m_doodads.begin();
                entry = i->first;
                filename = i->second;
                m_doodads.erase(i);
                isDoodad = true;
            }
        }

        auto const output = BuildAbsoluteFilename(m_outputPath, filename, entry);

        try
        {
            if (isDoodad)
            {
                const parser::Doodad doodad(filename);

                // ignore doodads with no collideable geometry
                if (doodad.Vertices.empty() || doodad.Indices.empty())
                {
                    std::lock_guard<std::mutex> guard(m_mutex);
                    m_serialized[entry] = "";
                    continue;
                }

                meshfiles::SerializeDoodad(&doodad, output);
            }
            else
            {
                const parser::Wmo wmo(filename);

                // ignore wmos with no collision geometry.  this probably shouldn't ever really happen
                if (wmo.Vertices.empty() || wmo.Indices.empty())
                {
                    std::lock_guard<std::mutex> guard(m_mutex);
                    m_serialized[entry] = "";
                    continue;
                }

                // note that this will also serialize all doodads referenced in all doodad sets within this wmo
                meshfiles::SerializeWmo(&wmo, output);
            }

            std::lock_guard<std::mutex> guard(m_mutex);
            m_serialized[entry] = output.filename().string();
        }
        catch (utility::exception const &e)
        {
            std::stringstream str;
            str << "ERROR: " << e.what() << "\n";
            std::cerr << str.str();
            std::cerr.flush();
        }
    }
}

