#include "GameObjectBVHBuilder.hpp"
#include "BVHConstructor.hpp"
#include "MeshBuilder.hpp"

#include "parser/MpqManager.hpp"
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

namespace parser
{
GameObjectBVHBuilder::GameObjectBVHBuilder(const std::string& dataPath, const std::string& outputPath, size_t workers)
    : m_dataPath(dataPath), m_bvhConstructor(outputPath), m_workers(workers), m_shutdownRequested(false)
{
    sMpqManager.Initialize(m_dataPath);

    const parser::DBC displayInfo("DBFilesClient\\GameObjectDisplayInfo.dbc");

    for (auto i = 0u; i < displayInfo.RecordCount(); ++i)
    {
        auto const row = displayInfo.GetField(i, 0);
        auto const path = displayInfo.GetStringField(i, 1);

        if (!path.length())
            continue;

        if (!sMpqManager.FileExists(path))
            continue;

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
    // if zero or one threads are requested, execute synchronously
    if (m_workers <= 1)
        Work();
    else
        for (auto i = 0u; i < m_workers; ++i)
            m_threads.push_back(std::thread(&GameObjectBVHBuilder::Work, this));
}

size_t GameObjectBVHBuilder::Shutdown()
{
    m_shutdownRequested = true;
    for (auto &thread : m_threads)
        thread.join();

    m_threads.clear();

    m_bvhConstructor.Shutdown();

    size_t result = 0;
    for (auto const &entry : m_serialized)
        if (entry.second != "")
            ++result;

    return result;
}

void GameObjectBVHBuilder::Work()
{
    sMpqManager.Initialize(m_dataPath);

    while (!m_shutdownRequested)
    {
        std::uint32_t entry;
        std::string filename;
        bool isDoodad;

        {
            std::lock_guard<std::mutex> guard(m_mutex);

            if (m_doodads.empty() && m_wmos.empty())
                break;

            // if there are any wmos, start them first.  they take longer and
            // if we are using multiple threads this will let them get started
            // sooner in the process.

            if (!m_wmos.empty())
            {
                auto const i = m_wmos.begin();
                entry = i->first;
                filename = i->second;
                m_wmos.erase(i);
                isDoodad = false;
            }
            // otherwise there must be a doodad or we would have bailed out, so
            // pick it and continue.
            else
            {
                auto const i = m_doodads.begin();
                entry = i->first;
                filename = i->second;
                m_doodads.erase(i);
                isDoodad = true;
            }
        }

        try
        {
            fs::path output;

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

                output = m_bvhConstructor.AddTemporaryObstacle(entry, filename);
                meshfiles::SerializeDoodad(doodad, output);
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

                output = m_bvhConstructor.AddTemporaryObstacle(entry, filename);

                // note that this will also serialize all doodads referenced in all doodad sets within this wmo
                meshfiles::SerializeWmo(wmo, m_bvhConstructor);
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
}