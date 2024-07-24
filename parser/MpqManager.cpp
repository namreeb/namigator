#include "MpqManager.hpp"

#include "DBC.hpp"
#include "StormLib.h"
#include "utility/Exception.hpp"
#include "utility/String.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace
{
std::uint32_t
GetRootAreaId(const std::unordered_map<std::uint32_t, std::uint32_t>& areas,
              std::uint32_t id)
{
    auto const i = areas.find(id);

    if (i == areas.end())
        THROW(Result::GETROOTAREAID_INVALID_ID);

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

void AddIfMpq(std::vector<fs::path>& files, const fs::path& file)
{
    if (!fs::is_regular_file(file))
        return;

    auto const extension = file.extension().string();

    if (extension[0] != '.' || extension.length() != 4)
        return;

    if ((extension[1] != 'M' && extension[1] != 'm') ||
        (extension[2] != 'P' && extension[2] != 'p') ||
        (extension[3] != 'Q' && extension[3] != 'q'))
        return;

    files.push_back(file);
}

std::string GetRealModelPath(const std::string& path, bool alpha)
{
    auto const length = path.length();

    if (length < 4)
        return path;
    if (path[length - 4] != '.')
        return path;
    if ((path[length - 3] != 'm' && path[length - 3] != 'M') ||
        (path[length - 2] != 'd' && path[length - 2] != 'D'))
        return path;

    if (alpha)
    {
        if (path[length - 1] != 'l' && path[length - 1] != 'L')
            return path;
        return path.substr(0, length - 4) + ".mdx";
    }
    else
    {
        if (path[length - 1] != 'x' && path[length - 1] != 'X' &&
            path[length - 1] != 'l' && path[length - 1] != 'L')
            return path;
        return path.substr(0, length - 4) + ".m2";
    }
}
} // namespace

namespace parser
{
thread_local MpqManager sMpqManager;

void MpqManager::LoadMpq(const fs::path& filePath)
{
    HANDLE archive;

    if (!SFileOpenArchive(filePath.string().c_str(), 0, MPQ_OPEN_READ_ONLY, &archive))
        THROW(Result::COULD_NOT_OPEN_MPQ).ErrorCode();

    std::error_code ec;
    auto const rel = fs::relative(filePath, BasePath, ec);
    if (ec)
        MpqHandles[filePath.string()] = archive;
    else
        MpqHandles[utility::lower(rel.string())] = archive;
}

void MpqManager::Initialize()
{
    Initialize(".");
}

// Priority logic is explained at https://github.com/namreeb/namigator/issues/22
void MpqManager::Initialize(const fs::path& wowDir)
{
    if (!fs::is_directory(wowDir))
        THROW(Result::NO_DATA_FILES_FOUND);

    Alpha = false;
    BasePath = fs::path(wowDir);
    std::string locale = "";

    for (auto const& d : fs::directory_iterator(BasePath))
    {
        if (!fs::is_directory(d.status()))
            continue;

        auto const dirString = d.path().filename().string();

        if (dirString.length() != 4)
            continue;

        // all locale directories should have files named lcLE\locale-lcLE.mpq
        // and lcLE\patch-lcLE*.mpq
        if (fs::exists(BasePath / dirString / ("locale-" + dirString + ".MPQ")))
        {
            locale = dirString;
            break;
        }
    }

    std::vector<fs::path> files;

    // if we are running on test data we do not expect to find a locale
    AddIfExists(files, BasePath / "test_map.mpq");

    AddIfExists(files, BasePath / "base.MPQ");
    AddIfExists(files, BasePath / "dbc.MPQ");
    AddIfExists(files, BasePath / "model.MPQ");
    AddIfExists(files, BasePath / "alternate.MPQ");
    AddIfExists(files, BasePath / ".." / "alternate.MPQ");
    AddIfExists(files, BasePath / "speech2.MPQ");
    AddIfExists(files, BasePath / ".." / "speech2.MPQ");

    for (auto i = 9; i > 0; --i)
    {
        std::stringstream s1;
        s1 << "patch-" << i << ".MPQ";
        AddIfExists(files, BasePath / s1.str());
    }

    for (auto i = 9; i > 0; --i)
    {
        std::stringstream s1;
        s1 << "patch-" << locale << "-" << i << ".MPQ";
        AddIfExists(files, BasePath / locale / s1.str());
    }

    // the alpha client stores data in lots of small MPQs inside the
    // Data/World directory
    if (fs::is_directory(BasePath / "World"))
    {
        Alpha = true;

        for (auto const& f :
                fs::recursive_directory_iterator(BasePath / "World"))
            AddIfMpq(files, f);
    }

    AddIfExists(files, BasePath / "patch.MPQ");
    AddIfExists(files, BasePath / locale / ("patch-" + locale + ".MPQ"));
    AddIfExists(files, BasePath / "wmo.MPQ");
    AddIfExists(files, BasePath / "expansion.MPQ");
    AddIfExists(files, BasePath / "lichking.MPQ");
    AddIfExists(files, BasePath / "common.MPQ");
    AddIfExists(files, BasePath / "common-2.MPQ");
    AddIfExists(files, BasePath / "terrain.MPQ");
    AddIfExists(files, BasePath / locale / ("locale-" + locale + ".MPQ"));
    AddIfExists(files, BasePath / locale / ("speech-" + locale + ".MPQ"));
    AddIfExists(files, BasePath / locale /
                            ("expansion-locale-" + locale + ".MPQ"));
    AddIfExists(files,
                BasePath / locale / ("lichking-locale-" + locale + ".MPQ"));
    AddIfExists(files, BasePath / locale /
                            ("expansion-speech-" + locale + ".MPQ"));
    AddIfExists(files,
                BasePath / locale / ("lichking-speech-" + locale + ".MPQ"));

    if (files.empty())
        THROW(Result::NO_DATA_FILES_FOUND);

    for (auto const& file : files)
        LoadMpq(file);

    const DBC maps("DBFilesClient\\Map.dbc");

    for (auto i = 0u; i < maps.RecordCount(); ++i)
    {
        auto const map_name = utility::lower(maps.GetStringField(i, 1));
        Maps[map_name] = maps.GetField(i, 0);
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
    for (auto const& i : MpqHandles)
        if (SFileHasFile(i.second, file.c_str()))
            return true;
    return false;
}

std::unique_ptr<utility::BinaryStream>
MpqManager::OpenFile(const std::string& file)
{
    if (MpqHandles.empty())
        THROW(Result::MPQ_MANAGER_NOT_INIATIALIZED);

    auto file_lower = utility::lower(GetRealModelPath(file, Alpha));

    for (auto const& i : MpqHandles)
    {
        if (!SFileHasFile(i.second, file_lower.c_str()))
            continue;

        HANDLE fileHandle;
        if (!SFileOpenFileEx(i.second, file_lower.c_str(), SFILE_OPEN_FROM_MPQ,
                             &fileHandle))
            THROW(Result::ERROR_IN_SFILEOPENFILEX).ErrorCode();

        auto const fileSize = SFileGetFileSize(fileHandle, nullptr);

        if (!fileSize)
            continue;

        std::vector<std::uint8_t> inFileData(fileSize);

        if (!SFileReadFile(fileHandle, &inFileData[0],
                           static_cast<DWORD>(inFileData.size()), nullptr,
                           nullptr))
        {
            SFileCloseFile(fileHandle);
            THROW(Result::ERROR_IN_SFILEREADFILE).ErrorCode();
        }

        SFileCloseFile(fileHandle);

        return std::make_unique<utility::BinaryStream>(inFileData);
    }

    // it is possible that we reach here when operating on alpha data, when
    // many (all?) files were in their own MPQ.  lets check for that next...
    file_lower += ".mpq";

    for (auto const& i : MpqHandles)
    {
        // if we are on Linux, the mpq filenames will use forward slashes
        // instead of backslashes.  this code could be cleaner.
        std::string mpqPath = i.first;
        std::replace(mpqPath.begin(), mpqPath.end(), '/', '\\');

        if (mpqPath != file_lower)
            continue;

        // if we have found a match, there should be exactly two files in this
        // mpq: the data file, and a checksum file.

        SFILE_FIND_DATA data;
        auto const search = SFileFindFirstFile(i.second, "*", &data, nullptr);
        if (!search)
            continue;

        // TODO: what follows is probably over kill, but left here to test the
        // assumptions the code relies on.
        std::vector<std::string> files;
        std::string candidate("");
        do
        {
            const std::string fn(data.cFileName);
            files.push_back(fn);

            if (fn != "(attributes)" && data.dwFileSize != 16 &&
                data.dwFileSize != 0)
            {
                if (!candidate.empty())
                    THROW(Result::MULTIPLE_CANDIDATES_IN_ALPHA_MPQ);
                candidate = fn;
            }

            if (!SFileFindNextFile(search, &data))
            {
                SFileFindClose(search);
                break;
            }
        } while (true);

        if (files.size() != 3)
            THROW(Result::TOO_MANY_FILES_IN_ALPHA_MPQ);
        if (candidate.empty())
            THROW(Result::NO_MPQ_CANDIDATE);

        HANDLE fileHandle;
        if (!SFileOpenFileEx(i.second, candidate.c_str(), SFILE_OPEN_FROM_MPQ,
                             &fileHandle))
            THROW(Result::ERROR_IN_SFILEOPENFILEX).ErrorCode();

        auto const fileSize = SFileGetFileSize(fileHandle, nullptr);

        std::vector<std::uint8_t> inFileData(fileSize);

        if (!SFileReadFile(fileHandle, &inFileData[0],
                           static_cast<DWORD>(inFileData.size()), nullptr,
                           nullptr))
        {
            SFileCloseFile(fileHandle);
            THROW(Result::ERROR_IN_SFILEREADFILE).ErrorCode();
        }

        SFileCloseFile(fileHandle);

        return std::make_unique<utility::BinaryStream>(inFileData);
    }

    return nullptr;
}

unsigned int MpqManager::GetMapId(const std::string& name) const
{
    std::string nameLower = utility::lower(name);

    auto const i = Maps.find(nameLower);

    if (i == Maps.end())
        THROW_MSG("Map ID for " + name + " not found", Result::MAP_ID_NOT_FOUND);

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
