#include "MpqManager.hpp"
#include "DBC.hpp"

#include "utility/Exception.hpp"
#include "StormLib.h"

#include <vector>
#include <sstream>
#include <cstdint>
#include <cctype>
#include <algorithm>
#include <map>
#include <experimental/filesystem>

namespace parser
{
std::list<HANDLE> MpqManager::MpqHandles;
std::string MpqManager::WowDir;
std::map<unsigned int, std::string> MpqManager::Maps;

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

// TODO examine how the retail clients determine MPQ loading order
void MpqManager::Initialize(const std::string &wowDir)
{
    WowDir = wowDir;

    auto wowPath = std::experimental::filesystem::path(wowDir);

    std::vector<std::experimental::filesystem::directory_entry> directories;
    std::vector<std::experimental::filesystem::path> files;
    std::vector<std::experimental::filesystem::path> patches;

    for (auto i = std::experimental::filesystem::directory_iterator(wowPath); i != std::experimental::filesystem::directory_iterator(); ++i)
    {
        if (std::experimental::filesystem::is_directory(i->status()))
        {
            directories.push_back(*i);
            continue;
        }

        if (!std::experimental::filesystem::is_regular_file(i->status()))
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

        std::vector<std::experimental::filesystem::path> localePatches;
        std::experimental::filesystem::path firstPatch;
        for (auto i = std::experimental::filesystem::directory_iterator(dir); i != std::experimental::filesystem::directory_iterator(); ++i)
        {
            if (std::experimental::filesystem::equivalent(*i, localeMpq))
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
            if (!std::experimental::filesystem::is_empty(firstPatch))
                files.push_back(firstPatch);

            std::sort(localePatches.begin(), localePatches.end());
            std::copy(localePatches.cbegin(), localePatches.cend(), std::back_inserter(files));
        }
    }

    for (auto const &file : files)
        LoadMpq(file.string());

    for (auto const &file : patches)
        for (auto const &handle : MpqHandles)
            if (!SFileOpenPatchArchive(handle, file.string().c_str(), "base", 0))
                THROW("Failed to apply patch").ErrorCode();

    DBC maps("DBFilesClient\\Map.dbc");

    for (auto i = 0u; i < maps.RecordCount(); ++i)
        Maps[maps.GetField(i, 0)] = maps.GetStringField(i, 1);
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

        auto const fileSize = SFileGetFileSize(fileHandle, nullptr);

        if (!fileSize)
            continue;

        std::vector<std::uint8_t> inFileData(fileSize);

        if (!SFileReadFile(fileHandle, &inFileData[0], static_cast<DWORD>(inFileData.size()), nullptr, nullptr))
            THROW("Error in SFileReadFile").ErrorCode();

        return new utility::BinaryStream(inFileData);
    }

    return nullptr;
}

unsigned int MpqManager::GetMapId(const std::string &name)
{
    for (auto const &entry : Maps)
    {
        std::string entryLower;
        std::transform(entry.second.begin(), entry.second.end(), std::back_inserter(entryLower), ::tolower);

        std::string nameLower;
        std::transform(name.begin(), name.end(), std::back_inserter(nameLower), ::tolower);

        if (entryLower == nameLower)
            return entry.first;
    }

    THROW("Map ID for " + name + " not found");
}
}
