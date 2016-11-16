#include "MpqManager.hpp"

#include "utility/Include/Exception.hpp"
#include "StormLib.h"

#ifdef WIN32
#include <Windows.h>
#endif

#ifdef DEBUG
#include <iostream>
#endif

#include <vector>
#include <list>
#include <sstream>
#include <cstdint>

namespace parser
{
std::list<HANDLE> MpqManager::MpqHandles;
std::string MpqManager::WowDir;

void MpqManager::LoadMpq(const std::string &filePath)
{
    HANDLE archive;
        
    if (!SFileOpenArchive(filePath.c_str(), 0, MPQ_OPEN_READ_ONLY, &archive))
        THROW("Could not open MPQ").ErrorCode();

    MpqHandles.push_front(archive);
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
    std::list<std::string> patches;
    std::list<std::string> files;

    do
    {
        std::string file = wowDir + "\\" + FindFileData.cFileName;
        file = file.substr(0, file.rfind('.'));

        if (file.find("wow-update") == std::string::npos)
            files.push_back(file);
        else
            patches.push_back(file);
    } while (FindNextFile(hFind, &FindFileData));
#else
#error "Not yet implemented"
#endif

    files.sort();
    patches.sort();

    while (!files.empty())
    {
        LoadMpq(files.front() + ".mpq");
        files.pop_front();
    }

    while (!patches.empty())
    {
        std::string patchFile = patches.front() + ".mpq";
        patches.pop_front();

        for (auto const &handle : MpqHandles)
            if (!SFileOpenPatchArchive(handle, patchFile.c_str(), "base", 0))
                THROW("Failed to apply patch").ErrorCode();
    }
}

utility::BinaryStream *MpqManager::OpenFile(const std::string &file)
{
    for (auto const &handle : MpqHandles)
    {
        // XXX - disabled temporarily due to bug, reported to ladik
        //if (!SFileHasFile(*i, (char *)file.c_str()))
        //    continue;

        HANDLE fileHandle;
        if (!SFileOpenFileEx(handle, file.c_str(), SFILE_OPEN_FROM_MPQ, &fileHandle))
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
                continue;
            else
                THROW("Error in SFileOpenFileEx").ErrorCode();
        }

        std::vector<std::uint8_t> inFileData(SFileGetFileSize(fileHandle, nullptr));

        if (!SFileReadFile(fileHandle, &inFileData[0], static_cast<DWORD>(inFileData.size()), nullptr, nullptr))
            THROW("Error in SFileReadFile").ErrorCode();

        return new utility::BinaryStream(inFileData);
    }

    return nullptr;
}
}