#include "Worker.hpp"

#include <thread>
#include <chrono>
#include <queue>
#include <mutex>
#include <utility>
#include <cmath>
#include <limits>
#include <vector>

#ifdef _DEBUG
#include <iostream>
#endif

Worker::Worker(pathfind::build::DataManager *dataManager)
    : m_shutdownRequested(false), m_thread([this]() { this->Work(); }), m_meshBuild(dataManager)
{}

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

            if (m_adts.empty())
                continue;

            adtXY = m_adts.front();
            m_adts.pop();
        }

        m_meshBuild.GenerateTile(adtXY.first, adtXY.second);
    } while (!m_shutdownRequested);
}

void Worker::EnqueueAdt(int x, int y)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    m_adts.push({ x, y });
}

int Worker::Jobs() const
{
    std::lock_guard<std::mutex> guard(m_mutex);
    return m_adts.size();
}

