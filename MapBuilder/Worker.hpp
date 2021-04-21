#pragma once

#include "MeshBuilder.hpp"

#include <thread>
#include <mutex>
#include <utility>
#include <atomic>

class Worker
{
    private:
        const std::string m_dataPath;

        MeshBuilder * const m_meshBuilder;

        const bool m_wmo;
        bool m_shutdownRequested;
        std::atomic_bool m_isFinished;

        std::thread m_thread;

        void Work();

    public:
        Worker(const std::string& dataPath, MeshBuilder *meshBuilder);
        ~Worker();

        bool IsFinished() const;
};