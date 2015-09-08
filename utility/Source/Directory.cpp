#include <string>

#include "Directory.hpp"

#ifdef WIN32
#include <Windows.h>
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#error "Directory class not implemented yet"
#define GetCurrentDir getcwd
#endif

namespace utility
{
    bool Directory::Exists(const std::string &path)
    {
        return Exists(path.c_str());
    }

    bool Directory::Exists(const char *path)
    {
        unsigned int a = GetFileAttributes(path);

        bool isDir = (a & FILE_ATTRIBUTE_DIRECTORY) ? true : false;

        return a == INVALID_FILE_ATTRIBUTES ? false : isDir;
    }

    void Directory::Create(const std::string &path)
    {
        Create(path.c_str());
    }

    void Directory::Create(const char *path)
    {
        CreateDirectory(path, NULL);
    }

    std::string Directory::Current()
    {
        char cCurrentDir[1024];

        return std::string(GetCurrentDir(cCurrentDir, sizeof(cCurrentDir)));
    }
}