#include "RecastContext.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

void RecastContext::doLog(const rcLogCategory category, const char* msg,
                          const int /*len*/)
{
    if (!m_logLevel || category < m_logLevel)
        return;

    std::stringstream out;

    out << "Thread #" << std::setfill(' ') << std::setw(6)
        << std::this_thread::get_id() << " ";

    switch (category)
    {
        case rcLogCategory::RC_LOG_ERROR:
            out << "ERROR: ";
            break;
        case rcLogCategory::RC_LOG_PROGRESS:
            out << "PROGRESS: ";
            break;
        case rcLogCategory::RC_LOG_WARNING:
            out << "WARNING: ";
            break;
        default:
            out << "rcContext::doLog(" << category << "): ";
            break;
    }

    out << msg << std::endl;
    std::cout << out.str();
}