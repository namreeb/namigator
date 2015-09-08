#pragma once

#include <string>
#include <list>
#include <map>

#ifdef WIN32
#include <windows.h>
#else
#error "UNIX HANDLE header unknown"
#endif

#include "BinaryStream.hpp"
#include "Cache.hpp"

namespace parser
{
    class MpqManager
    {
        private:
            static std::string WowRegDir();
            static std::list<HANDLE> MpqHandles;

            //static void LoadMpqs(const std::string filePaths[]);
            static void LoadMpq(const std::string &filePath);

        public:
            static std::string WowDir;
            static std::list<std::string> Archives;

            static void Initialize();
            static void Initialize(const std::string &wowDir);

            static utility::BinaryStream *OpenFile(const std::string &file);
    };
};