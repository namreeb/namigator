#pragma once

#include <exception>
#include <string>
#include <sstream>
#include <Windows.h>

#define THROW_BASE() throw utility::exception(::GetLastError(), __FILE__, __FUNCSIG__, __LINE__)
#define THROW(x) THROW_BASE().Message(x)

namespace utility
{
class exception : public virtual std::exception
{
    private:
        std::string _str;
        DWORD _err;

    public:
        exception(DWORD err, const char *file, const char *function, int line) : _err(err)
        {
            std::stringstream s;

            s << file << ":" << line << " (" << function << "):";

            _str = s.str();
        }

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