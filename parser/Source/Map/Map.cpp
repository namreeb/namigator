#include "MpqManager.hpp"
#include "Map/Map.hpp"
#include "Adt/Adt.hpp"
#include "Wmo/Wmo.hpp"
#include "Wmo/WmoPlacement.hpp"

#include "utility/Include/Directory.hpp"
#include "utility/Include/BinaryStream.hpp"
#include "utility/Include/Exception.hpp"
#include "utility/Include/LinearAlgebra.hpp"

#include <memory>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cassert>

namespace parser
{
Map::Map(const std::string &name) : Name(name), m_globalWmo(nullptr)
{
    std::string file = "World\\Maps\\" + Name + "\\" + Name + ".wdt";

    std::unique_ptr<utility::BinaryStream> reader(MpqManager::OpenFile(file));

    size_t mphdLocation;
    if (!reader->GetChunkLocation("MPHD", mphdLocation))
        THROW("MPHD not found");

    reader->SetPosition(mphdLocation + 8);

    auto const hasTerrain = !(reader->Read<unsigned int>() & 0x1);

    size_t mainLocation;
    if (!reader->GetChunkLocation("MAIN", mainLocation))
        THROW("MAIN not found");

    reader->SetPosition(mainLocation + 8);

    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x)
        {
            const int flag = reader->Read<int>();

            // skip async object
            reader->Slide(4);

            m_hasAdt[y][x] = !!(flag & 1);
        }

    // for worlds with terrain, parsing stops here.  else, load single wmo
    if (hasTerrain)
        return;

    size_t mwmoLocation;
    if (!reader->GetChunkLocation("MWMO", mwmoLocation))
        THROW("MWMO not found");
    reader->SetPosition(mwmoLocation + 8);

    auto const wmoName = reader->ReadCString();

    size_t modfLocation;
    if (!reader->GetChunkLocation("MODF", modfLocation))
        THROW("MODF not found");
    reader->SetPosition(modfLocation + 8);

    input::WmoPlacement placement;
    reader->ReadBytes(&placement, sizeof(placement));

    utility::BoundingBox bounds;
    placement.GetBoundingBox(bounds);

    utility::Vertex origin;
    placement.GetOrigin(origin);

    utility::Matrix transformMatrix;
    placement.GetTransformMatrix(transformMatrix);

    m_globalWmo.reset(new WmoInstance(GetWmo(wmoName), placement.DoodadSet, bounds, origin, transformMatrix));
}

bool Map::HasAdt(int x, int y) const
{
    return m_hasAdt[y][x];
}

const Adt *Map::GetAdt(int x, int y)
{
    std::lock_guard<std::mutex> guard(m_adtMutex);

    if (!m_hasAdt[y][x])
        return nullptr;

    if (!m_adts[y][x])
        m_adts[y][x].reset(new Adt(this, x, y));

    return m_adts[y][x].get();
}

void Map::UnloadAdt(int x, int y)
{
    std::lock_guard<std::mutex> guard(m_adtMutex);
    m_adts[y][x].reset(nullptr);
}

const Wmo *Map::GetWmo(const std::string &name)
{
    std::lock_guard<std::mutex> guard(m_wmoMutex);
    
    for (auto const &wmo : m_loadedWmos)
        if (wmo->FullPath == name)
            return wmo.get();

    auto ret = new Wmo(this, name);
    m_loadedWmos.push_back(std::unique_ptr<const Wmo>(ret));
    return ret;
}

void Map::InsertWmoInstance(unsigned int uniqueId, const WmoInstance *wmo)
{
    std::lock_guard<std::mutex> guard(m_wmoMutex);
    m_loadedWmoInstances[uniqueId].reset(wmo);
}

void Map::UnloadWmoInstance(unsigned int uniqueId)
{
    std::lock_guard<std::mutex> guard(m_wmoMutex);
    m_loadedWmoInstances.erase(uniqueId);
}

const WmoInstance *Map::GetWmoInstance(unsigned int uniqueId) const
{
    std::lock_guard<std::mutex> guard(m_wmoMutex);

    auto const itr = m_loadedWmoInstances.find(uniqueId);

    return itr == m_loadedWmoInstances.end() ? nullptr : itr->second.get();
}

const WmoInstance *Map::GetGlobalWmoInstance() const
{
    return m_globalWmo.get();
}

const Doodad *Map::GetDoodad(const std::string &name)
{
    std::lock_guard<std::mutex> guard(m_doodadMutex);

    for (auto const &doodad : m_loadedDoodads)
        if (doodad->Name.substr(0, doodad->Name.rfind('.')) == name.substr(0, name.rfind('.')))
            return doodad.get();

    auto ret = new Doodad(name);
    m_loadedDoodads.push_back(std::unique_ptr<const Doodad>(ret));
    return ret;
}

void Map::InsertDoodadInstance(unsigned int uniqueId, const DoodadInstance *doodad)
{
    std::lock_guard<std::mutex> guard(m_doodadMutex);
    m_loadedDoodadInstances[uniqueId].reset(doodad);
}

void Map::UnloadDoodadInstance(unsigned int uniqueId)
{
    std::lock_guard<std::mutex> guard(m_doodadMutex);
    m_loadedDoodadInstances.erase(uniqueId);
}

const DoodadInstance *Map::GetDoodadInstance(unsigned int uniqueId) const
{
    std::lock_guard<std::mutex> guard(m_doodadMutex);

    auto const itr = m_loadedDoodadInstances.find(uniqueId);

    return itr == m_loadedDoodadInstances.end() ? nullptr : itr->second.get();
}

#ifdef _DEBUG
bool Map::IsAdtLoaded(int x, int y) const
{
    std::lock_guard<std::mutex> guard(m_adtMutex);
    return !!m_adts[y][x];
}

void Map::WriteMemoryUsage(std::ostream &o) const
{
    {
        std::lock_guard<std::mutex> guard(m_wmoMutex);
        o << "WMOs: " << m_loadedWmos.size() << std::endl;

        std::vector<std::pair<const Wmo *, unsigned int>> wmoMemoryUsage;
        wmoMemoryUsage.reserve(m_loadedWmos.size());

        for (auto const &wmo : m_loadedWmos)
            wmoMemoryUsage.push_back({ wmo.get(), wmo->MemoryUsage() });

        std::sort(wmoMemoryUsage.begin(), wmoMemoryUsage.end(), [](const std::pair<const Wmo *, unsigned int> &a, const std::pair<const Wmo *, unsigned int> &b) { return a.second > b.second; });

        for (auto const &wmoUsage : wmoMemoryUsage)
            o << "0x" << std::hex << std::uppercase << reinterpret_cast<unsigned long>(wmoUsage.first) << ": " << std::dec << wmoUsage.second << std::endl;
    }

    // FIXME - TODO - add wmo instances

    {
        std::lock_guard<std::mutex> guard(m_doodadMutex);
        o << "Doodads: " << m_loadedDoodads.size() << std::endl;

        std::vector<std::pair<const Doodad *, unsigned int>> doodadMemoryUsage;
        doodadMemoryUsage.reserve(m_loadedDoodads.size());

        for (auto const &doodad : m_loadedDoodads)
            doodadMemoryUsage.push_back({ doodad.get(), doodad->MemoryUsage() });

        std::sort(doodadMemoryUsage.begin(), doodadMemoryUsage.end(), [](const std::pair<const Doodad *, unsigned int> &a, const std::pair<const Doodad *, unsigned int> &b) { return a.second > b.second; });

        for (auto const &doodadUsage : doodadMemoryUsage)
            o << "0x" << std::hex << std::uppercase << reinterpret_cast<unsigned long>(doodadUsage.first) << ": " << std::dec << doodadUsage.second << std::endl;
    }

    // FIXME - TODO - add doodad instances

    {
        std::lock_guard<std::mutex> guard(m_adtMutex);
        
        int count = 0;
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x)
                if (m_adts[y][x])
                    ++count;

        o << "ADTs: " << count << " size = " << (count * sizeof(Adt)) << std::endl;
    }
}
#endif
}