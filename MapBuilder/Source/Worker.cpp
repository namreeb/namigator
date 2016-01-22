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

Worker::Worker(MeshBuilder *meshBuilder) : m_shutdownRequested(false), m_meshBuilder(meshBuilder), m_wmo(false) {}

Worker::~Worker()
{
    m_shutdownRequested = true;
    m_thread.join();
}

void Worker::Work()
{
    do
    {
        std::this_thread::sleep_for(std::chrono::microseconds(100));

        std::pair<int, int> adtXY;

        // we need only lock the collection long enough to get our next adt task
        {
            std::lock_guard<std::mutex> guard(m_mutex);

            // if all we have to do is a wmo, do so and continue.
            // note that no further work should be added, but we delay shutting down the thread for simplicity
            if (m_wmo)
            {
                m_meshBuilder->GenerateAndSaveGlobalWMO();
                m_wmo = false;
                continue;
            }

            if (m_adts.empty())
                continue;

            adtXY = m_adts.front();

            std::stringstream str;
            str << "Thread #" << std::setfill(' ') << std::setw(6) << std::this_thread::get_id() << " ADT ("
                << std::setfill(' ') << std::setw(2) << adtXY.first << ", "
                << std::setfill(' ') << std::setw(2) << adtXY.second << ")...\n";
            std::cout << str.str();
        }

        m_meshBuilder->GenerateAndSaveTile(adtXY.first, adtXY.second);

        // let the builder know that we are done with these up to nine tiles.  it will decide when to unload them
        for (int y = adtXY.second - 1; y <= adtXY.second + 1; ++y)
            for (int x = adtXY.first - 1; x <= adtXY.first + 1; ++x)
                m_meshBuilder->RemoveReference(x, y);

        m_adts.remove(adtXY);
    } while (!m_shutdownRequested);

#ifdef _DEBUG
    std::stringstream str;
    str << "Thread #" << std::this_thread::get_id() << " finished.\n";
    std::cout << str.str();
#endif
}

void Worker::EnqueueADT(int x, int y)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    assert(!m_wmo);
    m_adts.push_back({ x, y });
}

void Worker::EnqueueGlobalWMO()
{
    assert(m_adts.empty());
    m_wmo = true;
}

void Worker::InitializeADTReferences()
{
    for (auto s : m_adts)
    {
        m_meshBuilder->AddReference(s.first - 1, s.second - 1); m_meshBuilder->AddReference(s.first - 0, s.second - 1); m_meshBuilder->AddReference(s.first + 1, s.second - 1);
        m_meshBuilder->AddReference(s.first - 1, s.second - 0); m_meshBuilder->AddReference(s.first - 0, s.second - 0); m_meshBuilder->AddReference(s.first + 1, s.second - 0);
        m_meshBuilder->AddReference(s.first - 1, s.second + 1); m_meshBuilder->AddReference(s.first - 0, s.second + 1); m_meshBuilder->AddReference(s.first + 1, s.second + 1);
    }
}

void Worker::Begin()
{
    m_thread = std::thread(&Worker::Work, this);
}

int Worker::Jobs() const
{
    std::lock_guard<std::mutex> guard(m_mutex);
    return m_adts.size();
}