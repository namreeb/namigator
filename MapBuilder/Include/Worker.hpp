#pragma once

#include "pathfind/Include/MeshBuilder.hpp"
#include "parser/Include/Output/Wmo.hpp"

#include <thread>
#include <list>
#include <mutex>
#include <utility>

class Worker
{
    private:
        pathfind::build::MeshBuilder * const m_meshBuilder;

        bool m_shutdownRequested;

        mutable std::mutex m_mutex;
        std::list<std::pair<int, int>> m_adts;
        bool m_wmo;

        // this is defined last so that the thread will not start until the other members are initialized
        std::thread m_thread;

        void Work();

    public:
        Worker(pathfind::build::MeshBuilder *meshBuilder);
        ~Worker();

        void EnqueueADT(int x, int y);
        void EnqueueGlobalWMO();

        void InitializeADTReferences();
        void Begin();

        int Jobs() const;
};