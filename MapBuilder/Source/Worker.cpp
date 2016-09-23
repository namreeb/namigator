#include "MeshBuilder.hpp"
#include "Worker.hpp"

#include "RecastDetourBuild/Include/Common.hpp"

#include <thread>
#include <chrono>
#include <mutex>
#include <utility>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <exception>

Worker::Worker(MeshBuilder *meshBuilder)
    : m_meshBuilder(meshBuilder), m_shutdownRequested(false),
      m_wmo(meshBuilder->IsGlobalWMO()), m_isRunning(false), m_thread(&Worker::Work, this) {}

Worker::~Worker()
{
    m_shutdownRequested = true;
    m_thread.join();
}

void Worker::Work()
{
    m_isRunning = true;

    try
    {
        do
        {
            int tileX, tileY;
            if (!m_meshBuilder->GetNextTile(tileX, tileY))
                break;

            if (m_wmo)
            {
                if (!m_meshBuilder->BuildAndSerializeWMOTile(tileX, tileY))
                {
                    std::stringstream error;
                    error << "Thread #" << std::setfill(' ') << std::setw(6) << std::this_thread::get_id() << " Global WMO tile ("
                        << std::setfill(' ') << std::setw(3) << tileX << ", "
                        << std::setfill(' ') << std::setw(3) << tileY << ") FAILED!  cell size = " << MeshSettings::CellSize << " voxels = " << MeshSettings::TileVoxelSize;
                    std::cout << error.str() << std::endl;
                    exit(1);
                }
            }
            else if (!m_meshBuilder->BuildAndSerializeADTTile(tileX, tileY))
            {
                auto const adtX = tileX / MeshSettings::TilesPerADT;
                auto const adtY = tileY / MeshSettings::TilesPerADT;
                auto const x = tileX % MeshSettings::TilesPerADT;
                auto const y = tileY % MeshSettings::TilesPerADT;

                std::stringstream error;
                error << "Thread #" << std::setfill(' ') << std::setw(6) << std::this_thread::get_id() << " ADT ("
                    << std::setfill(' ') << std::setw(2) << adtX << ", "
                    << std::setfill(' ') << std::setw(2) << adtY << ") tile ("
                    << x << ", " << y << ") FAILED!  cell size = " << MeshSettings::CellSize << " voxels = " << MeshSettings::TileVoxelSize;
                std::cout << error.str() << std::endl;
                exit(1);
            }
        } while (!m_shutdownRequested);
    }
    catch (std::exception const &e)
    {
        std::stringstream s;

        s << "\nThread #" << std::setfill(' ') << std::setw(6) << std::this_thread::get_id() << ": " << e.what() << "\n";

        std::cerr << s.str();
    }
    
#ifdef _DEBUG
    std::stringstream str;
    str << "Thread #" << std::this_thread::get_id() << " finished.\n";
    std::cout << str.str();
#endif

    m_isRunning = false;
}

bool Worker::IsRunning() const
{
    return m_isRunning;
}
