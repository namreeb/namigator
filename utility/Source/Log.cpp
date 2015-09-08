#include <sstream>

#include "Log.hpp"

namespace utility
{
    std::string Logger::Pop()
    {
        std::string ret = _logQueue.front();
        _logQueue.pop();

        return ret;
    }

    template <typename T>
    Logger & Logger::operator << (const T &t)
    {
        std::stringstream ss;

        ss << t << std::endl;

        _logQueue.push(ss.str());
    }
}