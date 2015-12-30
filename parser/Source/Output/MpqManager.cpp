#ifdef WIN32
#include <Windows.h>
#endif

#ifdef DEBUG
#include <iostream>
#endif

#include <vector>
#include <queue>
#include <list>
#include <sstream>

#include "StormLib.h"
#include "Output/MpqManager.hpp"

namespace parser
{
    std::list<HANDLE> MpqManager::MpqHandles;
    std::string MpqManager::WowDir;

    std::string MpqManager::WowRegDir()
    {
#if 0
#ifdef WIN32
        std::vector<char> path(1024);
        DWORD size = path.size();
        HKEY key;
        std::string ret;

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Blizzard Entertainment\\World of Warcraft"), 0, KEY_READ, &key) == ERROR_SUCCESS)
        {
            if (RegQueryValueExA(key, "InstallPath", nullptr, nullptr, (LPBYTE)&path[0], (LPDWORD)&size))
                THROW("Cannot find InstallPath key value");

            ret = std::string(&path[0]);
        }
        // if WoW is not installed
        else
            ret = std::string("H:\\World of Warcraft\\");

#else
        std::string ret = ".";
#endif
#else
        std::string ret = "D:\\Games\\World of Warcraft 1.12.1\\";
#endif

        return ret;
    }

    void MpqManager::LoadMpq(const std::string &filePath)
    {
        HANDLE archive;
        
        if (!SFileOpenArchive(filePath.c_str(), 0, MPQ_OPEN_READ_ONLY, &archive))
        {
            std::stringstream ss;
            ss << "Could not open " << filePath << " error: " << GetLastError() << std::endl;
            THROW(ss.str().c_str());
        }

        MpqHandles.push_back(archive);
    }

    void MpqManager::Initialize()
    {
        Initialize(WowRegDir());
    }

    void MpqManager::Initialize(const std::string &wowDir)
    {
        WowDir = wowDir;

#ifdef WIN32
        WIN32_FIND_DATA FindFileData;

        std::string dir = wowDir + "Data\\*.MPQ";

        HANDLE hFind = FindFirstFile(dir.c_str(), &FindFileData);

        if (hFind == INVALID_HANDLE_VALUE)
            THROW("FindFirstFile failed");

        // build a list of patches to open after all mpqs are open
        std::queue<std::string> patches;

        do
        {
            std::string file = wowDir + "Data\\" + FindFileData.cFileName;

            if (file.find("wow-update") == file.npos)
                LoadMpq(file);
            else
                patches.push(file);
        } while (FindNextFile(hFind, &FindFileData));
#else
#error "Not yet implemented"
#endif

        while (!patches.empty())
        {
            std::string patchFile = patches.front();
            patches.pop();

            for (std::list<HANDLE>::iterator i = MpqHandles.begin(); i != MpqHandles.end(); ++i)
            {
                if (!SFileOpenPatchArchive(*i, patchFile.c_str(), "base", 0))
                    THROW("Failed to apply patch");
            }
        }
    }

    utility::BinaryStream *MpqManager::OpenFile(const std::string &file)
    {
        //const char *fileData;
        //unsigned long fileSize;

        //if (MpqCache::Get(file, &fileData, &fileSize))
        //    return new utility::BinaryStream(fileData, fileSize);

        // at this point, we know the file is not cached.  read it in, and cache it.
        std::list<HANDLE>::iterator i;

        for (i = MpqHandles.begin(); i != MpqHandles.end(); ++i)
        {
            // XXX - disabled temporarily due to bug, reported to ladik
            //if (!SFileHasFile(*i, (char *)file.c_str()))
            //    continue;

            HANDLE fileHandle;
            if (!SFileOpenFileEx(*i, file.c_str(), SFILE_OPEN_FROM_MPQ, &fileHandle))
            {
                if (GetLastError() == ERROR_FILE_NOT_FOUND)
                    continue;
                else
                    THROW("Error in SFileOpenFileEx");
            }

            unsigned int fileSize = SFileGetFileSize(fileHandle, nullptr);

            std::vector<char> inFileData(fileSize);

            inFileData.resize(fileSize);

            if (!SFileReadFile(fileHandle, &inFileData[0], fileSize, nullptr, nullptr))
                THROW("Error in SFileReadFile");

            // insert the file data into the cache
            //MpqCache::Insert(file, inFileData, fileSize);

            return new utility::BinaryStream(inFileData, fileSize);
        }

        return nullptr;
    }
}