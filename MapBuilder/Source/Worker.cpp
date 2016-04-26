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

Worker::Worker(MeshBuilder *meshBuilder, bool globalWmo) : m_meshBuilder(meshBuilder), m_shutdownRequested(false), m_wmo(globalWmo), m_isRunning(false), m_thread(&Worker::Work, this) {}

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
            // if all we have to do is a wmo, do so and continue.
            // note that no further work should be added, but we delay shutting down the thread for simplicity
            if (m_wmo)
            {
                if (!m_meshBuilder->GenerateAndSaveGlobalWMO())
                    std::cout << "Failed to build global WMO " << std::endl;

                break;
            }

            int tileX, tileY;
            if (!m_meshBuilder->GetNextTile(tileX, tileY))
                break;

            auto const adtX = tileX / MeshSettings::TilesPerADT;
            auto const adtY = tileY / MeshSettings::TilesPerADT;
            auto const x = tileX % MeshSettings::TilesPerADT;
            auto const y = tileY % MeshSettings::TilesPerADT;

            //std::stringstream str;
            //str << "Thread #" << std::setfill(' ') << std::setw(6) << std::this_thread::get_id() << " ADT ("
            //    << std::setfill(' ') << std::setw(2) << adtX << ", "
            //    << std::setfill(' ') << std::setw(2) << adtY << ") tile ("
            //    << x << ", " << y << ") ... " << std::setprecision(3) << m_meshBuilder->PercentComplete() << "%\n";
            //std::cout << str.str();

            if (!m_meshBuilder->GenerateAndSaveTile(tileX, tileY))
            {
                // XXX FIXME - the code changes here are only temporary
                std::stringstream error;
                error << "Thread #" << std::setfill(' ') << std::setw(6) << std::this_thread::get_id() << " ADT ("
                      << std::setfill(' ') << std::setw(2) << adtX << ", "
                      << std::setfill(' ') << std::setw(2) << adtY << ") tile ("
                      << x << ", " << y << ")... FAILED!  cell size = " << MeshSettings::CellSize << " voxels = " << MeshSettings::TileVoxelSize << "\n";
                std::cout << error.str();
                exit(1);
            }
        } while (!m_shutdownRequested);
    }
    catch (std::exception const &e)
    {
        std::stringstream s;

        s << "\nThread #" << std::setfill(' ') << std::setw(6) << std::this_thread::get_id() << e.what() << "\n";

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
