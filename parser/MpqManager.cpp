#include "MpqManager.hpp"

#include "DBC.hpp"
#include "StormLib.h"
#include "utility/Exception.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

namespace
{
std::uint32_t
GetRootAreaId(const std::unordered_map<std::uint32_t, std::uint32_t>& areas,
              std::uint32_t id)
{
    auto const i = areas.find(id);

    if (i == areas.end())
        THROW("GetRootAreaId invalid id");

    // roots have no parent
    if (i->second == 0)
        return i->first;

    // roots are the root of the parent
    return GetRootAreaId(areas, i->second);
}

void AddIfExists(std::vector<fs::path>& files, const fs::path& file)
{
    if (fs::exists(file))
        files.push_back(file);
}
} // namespace

namespace parser
{
thread_local MpqManager sMpqManager;

void MpqManager::LoadMpq(const std::string& filePath)
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

void MpqManager::Initialize(const std::string& wowDir)
{
    auto const wowPath = fs::path(wowDir);
    std::string locale = "";

    for (auto i = fs::directory_iterator(wowPath);
         i != fs::directory_iterator(); ++i)
    {
        if (!fs::is_directory(i->status()))
            continue;

        auto const dirString = i->path().filename().string();

        if (dirString.length() != 4)
            continue;

        // all locale directories should have files named lcLE\locale-lcLE.mpq
        // and lcLE\patch-lcLE*.mpq
        if (fs::exists(wowPath / dirString / ("locale-" + dirString + ".MPQ")))
        {
            locale = dirString;
            break;
        }
    }

    std::vector<fs::path> files;

    // if we are running on test data we do not expect to find a locale
    if (locale.empty())
    {
        AddIfExists(files, wowPath / "test_map.MPQ");
    }
    else
    {
        AddIfExists(files, wowPath / "base.MPQ");
        AddIfExists(files, wowPath / "alternate.MPQ");
        AddIfExists(files, wowPath / ".." / "alternate.MPQ");

        for (auto i = 9; i > 0; --i)
        {
            std::stringstream s1;
            s1 << "patch-" << i << ".MPQ";
            AddIfExists(files, wowPath / s1.str());

            std::stringstream s2;
            s2 << "patch-" << locale << "-" << i << ".MPQ";
            AddIfExists(files, wowPath / s2.str());
        }

        AddIfExists(files, wowPath / locale / ("patch-" + locale + ".MPQ"));
        AddIfExists(files, wowPath / "patch.MPQ");
        AddIfExists(files, wowPath / "expansion.MPQ");
        AddIfExists(files, wowPath / "common.MPQ");
        AddIfExists(files, wowPath / locale / ("locale-" + locale + ".MPQ"));
        AddIfExists(files, wowPath / locale / ("speech-" + locale + ".MPQ"));
        AddIfExists(files,
                    wowPath / locale / ("expansion-locale-" + locale + ".MPQ"));
        AddIfExists(files,
                    wowPath / locale / ("expansion-speech-" + locale + ".MPQ"));
    }

    if (files.empty())
        THROW("No data files found");

    for (auto const& file : files)
        LoadMpq(file.string());

    const DBC maps("DBFilesClient\\Map.dbc");

    for (auto i = 0u; i < maps.RecordCount(); ++i)
    {
        auto const map_name = maps.GetStringField(i, 1);
        std::string map_name_lower;
        std::transform(map_name.begin(), map_name.end(),
                       std::back_inserter(map_name_lower), ::tolower);

        Maps[map_name_lower] = maps.GetField(i, 0);
    }

    const DBC area("DBFilesClient\\AreaTable.dbc");
    std::unordered_map<std::uint32_t, std::uint32_t> areaToZone;
    for (auto i = 0; i < area.RecordCount(); ++i)
    {
        auto const id = area.GetField(i, 0);
        auto const parent = area.GetField(i, 2);

        areaToZone[id] = parent;
    }

    for (auto const& i : areaToZone)
        AreaToZone[i.first] = GetRootAreaId(areaToZone, i.first);
}

bool MpqManager::FileExists(const std::string& file) const
{
    for (auto const& handle : MpqHandles)
        if (SFileHasFile(handle, file.c_str()))
            return true;
    return false;
}

std::unique_ptr<utility::BinaryStream>
MpqManager::OpenFile(const std::string& file)
{
    if (MpqHandles.empty())
        THROW("MpqManager not initialized");

    for (auto const& handle : MpqHandles)
    {
        if (!SFileHasFile(handle, file.c_str()))
            continue;

        HANDLE fileHandle;
        if (!SFileOpenFileEx(handle, file.c_str(), SFILE_OPEN_FROM_MPQ,
                             &fileHandle))
            THROW("Error in SFileOpenFileEx").ErrorCode();

        auto const fileSize = SFileGetFileSize(fileHandle, nullptr);

        if (!fileSize)
            continue;

        std::vector<std::uint8_t> inFileData(fileSize);

        if (!SFileReadFile(fileHandle, &inFileData[0],
                           static_cast<DWORD>(inFileData.size()), nullptr,
                           nullptr))
        {
            SFileCloseFile(fileHandle);
            THROW("Error in SFileReadFile").ErrorCode();
        }

        SFileCloseFile(fileHandle);

        return std::make_unique<utility::BinaryStream>(inFileData);
    }

    return nullptr;
}

unsigned int MpqManager::GetMapId(const std::string& name) const
{
    std::string nameLower;
    std::transform(name.begin(), name.end(), std::back_inserter(nameLower),
                   ::tolower);

    auto const i = Maps.find(nameLower);

    if (i == Maps.end())
        THROW("Map ID for " + name + " not found");

    return i->second;
}

unsigned int MpqManager::GetZoneId(unsigned int areaId) const
{
    auto const i = AreaToZone.find(areaId);

    if (i == AreaToZone.end())
        return 0;

    return i->second;
}
} // namespace parser
