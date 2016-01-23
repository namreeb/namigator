#pragma once

#include "MeshBuilder.hpp"

#include "parser/Include/Output/Wmo.hpp"

#include <thread>
#include <list>
#include <mutex>
#include <utility>

class Worker
{
    private:
        MeshBuilder * const m_meshBuilder;

        bool m_shutdownRequested;
        bool m_wmo;
        bool m_isRunning;

        std::thread m_thread;

        void Work();

    public:
        Worker(MeshBuilder *meshBuilder);
        ~Worker();

        void EnqueueGlobalWMO();
        void Begin();

        bool IsRunning() const;
};