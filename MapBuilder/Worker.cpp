#include "Worker.hpp"

#include <thread>
#include <chrono>
#include <queue>
#include <mutex>
#include <utility>

#ifdef _DEBUG
#include <iostream>
#endif

Worker::Worker(parser::Continent *continent)
    : m_shutdown(false), m_thread([this]() { this->Work(); }), m_continent(continent)
{}

Worker::~Worker()
{
    m_shutdown = true;
    m_thread.join();
}

void Worker::Work()
{
    while (!m_shutdown)
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

        auto const adt = m_continent->LoadAdt(adtXY.first, adtXY.second);

#ifdef _DEBUG
        std::cout << "We are now ready to process ADT (" << adtXY.first << ", " << adtXY.second << ")" << std::endl;
#endif
    }
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