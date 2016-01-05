#include "utility/Include/Directory.hpp"

#include <string>

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
    const unsigned int a = GetFileAttributes(path.c_str());
    const bool isDir = (a & FILE_ATTRIBUTE_DIRECTORY) ? true : false;

    return a == INVALID_FILE_ATTRIBUTES ? false : isDir;
}

void Directory::Create(const std::string &path)
{
    CreateDirectory(path.c_str(), nullptr);
}

std::string Directory::Current()
{
    char cCurrentDir[1024];

    return std::string(GetCurrentDir(cCurrentDir, sizeof(cCurrentDir)));
}
}