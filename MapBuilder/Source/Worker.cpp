#include "MeshBuilder.hpp"
#include "Worker.hpp"

#include <thread>
#include <chrono>
#include <queue>
#include <mutex>
#include <utility>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <exception>

Worker::Worker(MeshBuilder *meshBuilder) : m_meshBuilder(meshBuilder), m_shutdownRequested(false), m_wmo(false), m_isRunning(false) {}

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
                m_meshBuilder->GenerateAndSaveGlobalWMO();
                break;
            }

            int adtX, adtY;
            if (!m_meshBuilder->GetNextAdt(adtX, adtY))
                break;

            std::stringstream str;
            str << "Thread #" << std::setfill(' ') << std::setw(6) << std::this_thread::get_id() << " ADT ("
                << std::setfill(' ') << std::setw(2) << adtX << ", "
                << std::setfill(' ') << std::setw(2) << adtY << ")...\n";
            std::cout << str.str();

            m_meshBuilder->GenerateAndSaveTile(adtX, adtY);

            // let the builder know that we are done with these up to nine tiles.  it will decide when to unload them
            for (int y = adtY - 1; y <= adtY + 1; ++y)
                for (int x = adtX - 1; x <= adtX + 1; ++x)
                    m_meshBuilder->RemoveReference(x, y);
        } while (!m_shutdownRequested);
    }
    catch (std::exception const &e)
    {
        std::stringstream s;

        s << "Thread #" << std::setfill(' ') << std::setw(6) << std::this_thread::get_id() << e.what() << "\n";

        std::cerr << s.str();
    }
    
#ifdef _DEBUG
    std::stringstream str;
    str << "Thread #" << std::this_thread::get_id() << " finished.\n";
    std::cout << str.str();
#endif

    m_isRunning = false;
}

void Worker::EnqueueGlobalWMO()
{
    m_wmo = true;
}

void Worker::Begin()
{
    m_thread = std::thread(&Worker::Work, this);
}

bool Worker::IsRunning() const
{
    return m_isRunning;
}