#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>
#include <vector>
#include <thread>
#include <mutex>

// TODO: make generic Builder class for this and MeshBuilder?
class GameObjectBVHBuilder
{
    private:
        const std::string m_outputPath;
        const size_t m_workers;

        std::unordered_map<std::uint32_t, std::string> m_doodads;
        std::unordered_map<std::uint32_t, std::string> m_wmos;
        std::unordered_map<std::uint32_t, std::string> m_serialized;
        std::vector<std::thread> m_threads;

        mutable std::mutex m_mutex;

        bool m_shutdownRequested;

        std::string BuildAbsoluteFilename(const std::string &in) const;

        void Work();

    public:
        GameObjectBVHBuilder(const std::string &outputPath, int workers);
        ~GameObjectBVHBuilder();

        void Begin();
        void Shutdown();

        void WriteIndexFile() const;

        size_t Remaining() const { std::lock_guard<std::mutex> guard(m_mutex); return m_doodads.size() + m_wmos.size(); }
};