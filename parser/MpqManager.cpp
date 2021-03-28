#include "MpqManager.hpp"
#include "DBC.hpp"

#include "utility/Exception.hpp"
#include "StormLib.h"

#include <vector>
#include <sstream>
#include <cstdint>
#include <cctype>
#include <algorithm>
#include <unordered_map>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace parser
{
thread_local MpqManager sMpqManager;

void MpqManager::LoadMpq(const std::string &filePath)
{
    HANDLE archive;
        
    if (!SFileOpenArchive(filePath.c_str(), 0, MPQ_OPEN_READ_ONLY, &archive))
        THROW("Could not open MPQ").ErrorCode();

    MpqHandles.push_back(archive);
}

void MpqManager::Initialize()
{
    Initialize(".");
}

// TODO examine how the retail clients determine MPQ loading order
void MpqManager::Initialize(const std::string &wowDir)
{
    auto const wowPath = fs::path(wowDir);

    std::vector<fs::directory_entry> directories;
    std::vector<fs::path> files;
    std::vector<fs::path> patches;

    for (auto i = fs::directory_iterator(wowPath); i != fs::directory_iterator(); ++i)
    {
        if (fs::is_directory(i->status()))
        {
            directories.push_back(*i);
            continue;
        }

        if (!fs::is_regular_file(i->status()))
            continue;

        auto path = i->path().string();
        std::transform(path.begin(), path.end(), path.begin(), ::tolower);

        if (path.find(".mpq") == std::string::npos)
            continue;

        if (path.find("wow-update") == std::string::npos)
            files.push_back(i->path());
        else
            patches.push_back(i->path());
    }

    if (files.empty() && patches.empty())
        THROW("Found no MPQs");

    std::sort(files.begin(), files.end());
    std::sort(patches.begin(), patches.end());

    // all locale directories should have files named lcLE\locale-lcLE.mpq and lcLE\patch-lcLE*.mpq
    for (auto const &dir : directories)
    {
        auto const dirString = dir.path().filename().string();

        if (dirString.length() != 4)
            continue;

        auto const localeMpq = wowPath / dirString / ("locale-" + dirString + ".MPQ");
        auto found = false;

        std::vector<fs::path> localePatches;
        fs::path firstPatch;
        for (auto i = fs::directory_iterator(dir); i != fs::directory_iterator(); ++i)
        {
            if (fs::equivalent(*i, localeMpq))
                found = true;

            auto const filename = i->path().filename().string();

            if (filename.find("patch-" + dirString + "-") != std::string::npos)
                localePatches.push_back(i->path());
            else if (filename.find("patch-" + dirString) != std::string::npos)
                firstPatch = i->path();
        }

        if (found)
        {
            files.push_back(localeMpq);
            if (!fs::is_empty(firstPatch))
                files.push_back(firstPatch);

            std::sort(localePatches.begin(), localePatches.end());
            std::copy(localePatches.cbegin(), localePatches.cend(), std::back_inserter(files));
        }
    }

    // the current belief is that the game uses files from mpqs in reverse alphabetical order
    // this still needs to be checked.
    std::reverse(std::begin(files), std::end(files));
    std::reverse(std::begin(patches), std::end(patches));

    for (auto const &file : files)
        LoadMpq(file.string());

    for (auto const &file : patches)
        for (auto const &handle : MpqHandles)
            if (!SFileOpenPatchArchive(handle, file.string().c_str(), "base", 0))
                THROW("Failed to apply patch").ErrorCode();

    DBC maps("DBFilesClient\\Map.dbc");

    for (auto i = 0u; i < maps.RecordCount(); ++i)
    {
        auto const map_name = maps.GetStringField(i, 1);
        std::string map_name_lower;
        std::transform(map_name.begin(), map_name.end(), std::back_inserter(map_name_lower), ::tolower);

        Maps[map_name_lower] = maps.GetField(i, 0);
    }
}

utility::BinaryStream *MpqManager::OpenFile(const std::string &file)
{
    if (MpqHandles.empty())
        THROW("MpqManager not initialized");

    for (auto const &handle : MpqHandles)
    {
        if (!SFileHasFile(handle, file.c_str()))
            continue;

        HANDLE fileHandle;
        if (!SFileOpenFileEx(handle, file.c_str(), SFILE_OPEN_FROM_MPQ, &fileHandle))
            THROW("Error in SFileOpenFileEx").ErrorCode();

        auto const fileSize = SFileGetFileSize(fileHandle, nullptr);

        if (!fileSize)
            continue;

        std::vector<std::uint8_t> inFileData(fileSize);

        if (!SFileReadFile(fileHandle, &inFileData[0], static_cast<DWORD>(inFileData.size()), nullptr, nullptr))
        {
            SFileCloseFile(fileHandle);
            THROW("Error in SFileReadFile").ErrorCode();
        }

        SFileCloseFile(fileHandle);

        return new utility::BinaryStream(inFileData);
    }

    return nullptr;
}

unsigned int MpqManager::GetMapId(const std::string &name)
{
    std::string nameLower;
    std::transform(name.begin(), name.end(), std::back_inserter(nameLower), ::tolower);

    auto const i = Maps.find(nameLower);

    if (i == Maps.end())
        THROW("Map ID for " + name + " not found");

    return i->second;
}
}
