#pragma once

#include <exception>
#include <string>
#include <sstream>

#ifdef WIN32
#include <Windows.h>
#define THROW_BASE() throw utility::exception(::GetLastError(), __FILE__, __FUNCSIG__, __LINE__)
#else
#define THROW_BASE() throw utility::exception(__FILE__, __func__, __LINE__)
#endif

#define THROW(x) THROW_BASE().Message(x)

namespace utility
{
class exception : public virtual std::exception
{
    private:
        std::string _str;

#ifdef WIN32
        unsigned int _err;
#endif

    public:
#ifdef WIN32
        exception(unsigned int err, const char *file, const char *function, int line) : _err(err)
#else
        exception(const char *file, const char *function, int line)
#endif
        {
            std::stringstream s;

            s << file << ":" << line << " (" << function << "):";

            _str = s.str();
        }

#ifdef WIN32
        exception &ErrorCode()
        {
            char buff[1024];
            constexpr size_t buffSize = sizeof(buff) / sizeof(buff[0]);
            FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK, nullptr, _err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buff, buffSize, nullptr);

            std::stringstream s;

            s << _str << " error " << _err << " (" << buff << ")";

            _str = s.str();

            return *this;
        }
#else
        exception &ErrorCode() { return *this; }
#endif

        exception &Message(const std::string &msg)
        {
            std::stringstream s;

            s << _str << " " << msg;

            _str = s.str();

            return *this;
        }

        const char *what() const noexcept override { return _str.c_str(); }
};
}