#pragma once

#include <string>
#include <queue>

namespace utility
{
    class Logger
    {
        private:
            std::queue<std::string> _logQueue;

        public:
            inline std::string Pop();

            template <typename T>
            Logger& operator << (const T &t);
    };
}