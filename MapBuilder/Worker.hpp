#pragma once

#include "pathfind/Include/MeshBuilder.hpp"
#include "pathfind/Include/DataManager.hpp"

#include <thread>
#include <queue>
#include <mutex>
#include <utility>
#include <vector>

class Worker
{
    private:
        pathfind::build::MeshBuilder m_meshBuild;

        bool m_shutdownRequested;

        void Work();

        mutable std::mutex m_mutex;
        std::queue<std::pair<int, int>> m_adts;

        // this is defined last so that the thread will not start until the other members are initialized
        std::thread m_thread;


    public:
        Worker(pathfind::build::DataManager *dataManager);
        ~Worker();

        void EnqueueAdt(int x, int y);
        int Jobs() const;
};