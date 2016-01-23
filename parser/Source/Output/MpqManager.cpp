#include "Output/MpqManager.hpp"
#include "utility/Include/Exception.hpp"
#include "StormLib.h"

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

namespace parser
{
std::list<HANDLE> MpqManager::MpqHandles;
std::string MpqManager::WowDir;

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
    Initialize(".");
}

void MpqManager::Initialize(const std::string &wowDir)
{
    WowDir = wowDir;

#ifdef WIN32
    WIN32_FIND_DATA FindFileData;

    std::string dir = wowDir + "\\*.MPQ";

    HANDLE hFind = FindFirstFile(dir.c_str(), &FindFileData);

    if (hFind == INVALID_HANDLE_VALUE)
        THROW("FindFirstFile failed");

    // build a list of patches to open after all mpqs are open
    std::queue<std::string> patches;

    do
    {
        std::string file = wowDir + "\\" + FindFileData.cFileName;

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