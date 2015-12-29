#pragma once

#include "Output/Continent.hpp"

#include <thread>
#include <queue>
#include <mutex>
#include <utility>

class Worker
{
    private:
        bool m_shutdown;

        void Work();

        parser::Continent *const m_continent;

        mutable std::mutex m_mutex;
        std::queue<std::pair<int, int>> m_adts;

        // this is defined last so that the thread will not start until the other members are initialized
        std::thread m_thread;

    public:
        Worker(parser::Continent *continent);
        ~Worker();

        void EnqueueAdt(int x, int y);
        int Jobs() const;
};