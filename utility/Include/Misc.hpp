#pragma once

#define SLEEP_TIME    10

#define EXCEPTION_BUFFER_SIZE    1024
#ifdef WIN32
#include <windows.h>

#define THROW(x) {                                                                              \
    char buff[EXCEPTION_BUFFER_SIZE], filenameBuff[EXCEPTION_BUFFER_SIZE], *p, *q;              \
                                                                                                \
    sprintf_s(filenameBuff, EXCEPTION_BUFFER_SIZE, "%s", __FILE__);                             \
                                                                                                \
    for (p = filenameBuff; *p; ++p)                                                             \
    {                                                                                           \
        if (*p == '\\' || *p == '/')                                                            \
            q = p;                                                                              \
    }                                                                                           \
                                                                                                \
    sprintf_s(buff, EXCEPTION_BUFFER_SIZE, "%s:%d(%s): %s", (q+1), __LINE__, __FUNCTION__, x);  \
                                                                                                \
    throw buff;                                                                                 \
}
#else
#error "UNIX header unknown for Sleep()"
#define THROW(x) {char buff[1024]; snprintf(buff, 1024, "%s:%d(%s): %s", __FILE__, __LINE__, __FUNCTION__, x); throw buff;}
#endif

#include <string>

inline unsigned int Uint32Hash(unsigned int value)
{
    return value;
}

inline unsigned int StringHash(const std::string &value)
{
    unsigned int hash = 5381;
    const char *str = value.c_str(), *p;

    for (p = str; *p; ++p)
        hash = hash * 33 ^ *p;

    return hash;
}