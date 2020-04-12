#pragma once

#include "MeshBuilder.hpp"

#include <thread>
#include <list>
#include <mutex>
#include <utility>

class Worker
{
    private:
        const std::string m_dataPath;

        MeshBuilder * const m_meshBuilder;

        const bool m_wmo;
        bool m_shutdownRequested;
        bool m_isRunning;

        std::thread m_thread;

        void Work();

    public:
        Worker(const std::string& dataPath, MeshBuilder *meshBuilder);
        ~Worker();

        bool IsRunning() const;
};