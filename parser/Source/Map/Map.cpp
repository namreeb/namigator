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
#include <ostream>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <cstdint>

namespace parser
{
Map::Map(const std::string &name) : Name(name), m_globalWmo(nullptr)
{
    auto const file = "World\\Maps\\" + Name + "\\" + Name + ".wdt";

    std::unique_ptr<utility::BinaryStream> reader(MpqManager::OpenFile(file));

    size_t mphdLocation;
    if (!reader->GetChunkLocation("MPHD", mphdLocation))
        THROW("MPHD not found");

    reader->SetPosition(mphdLocation + 8);

    m_hasTerrain = !(reader->Read<unsigned int>() & 0x1);

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
    if (m_hasTerrain)
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

    utility::Matrix transformMatrix;
    placement.GetTransformMatrix(transformMatrix);

    m_globalWmo.reset(new WmoInstance(GetWmo(wmoName), placement.DoodadSet, bounds, transformMatrix));
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
    auto filename = name.substr(name.rfind('\\') + 1);
    filename = filename.substr(0, filename.rfind('.'));

    std::lock_guard<std::mutex> guard(m_wmoMutex);
    
    for (auto const &wmo : m_loadedWmos)
        if (wmo->FileName == filename)
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
    auto filename = name.substr(name.rfind('\\') + 1);
    filename = filename.substr(0, filename.rfind('.'));

    std::lock_guard<std::mutex> guard(m_doodadMutex);

    for (auto const &doodad : m_loadedDoodads)
        if (doodad->FileName == filename)
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

const DoodadInstance *Map::GetDoodadInstance(unsigned int uniqueId) const
{
    std::lock_guard<std::mutex> guard(m_doodadMutex);

    auto const itr = m_loadedDoodadInstances.find(uniqueId);

    return itr == m_loadedDoodadInstances.end() ? nullptr : itr->second.get();
}

void Map::Serialize(std::ostream& stream) const
{
    const std::uint32_t magic = 'MAP1';
    stream.write(reinterpret_cast<const char *>(&magic), sizeof(magic));

    if (!m_hasTerrain)
    {
        stream << (std::uint8_t)0;

        auto const wmo = GetGlobalWmoInstance();

        const std::uint32_t id = 0xFFFFFFFF;
        stream.write(reinterpret_cast<const char *>(&id), sizeof(id));

        const std::uint16_t doodadSet = static_cast<std::uint16_t>(wmo->DoodadSet);
        stream.write(reinterpret_cast<const char *>(&doodadSet), sizeof(doodadSet));

        stream << wmo->TransformMatrix;
        stream << wmo->Bounds;
        stream << std::left << std::setw(64) << std::setfill('\000') << wmo->Model->FileName;

        return;
    }

    stream << (std::uint8_t)1;

    {
        std::lock_guard<std::mutex> guard(m_wmoMutex);

        const std::uint32_t wmoInstanceCount = static_cast<std::uint32_t>(m_loadedWmoInstances.size());
        stream.write(reinterpret_cast<const char *>(&wmoInstanceCount), sizeof(wmoInstanceCount));

        for (auto const &wmo : m_loadedWmoInstances)
        {
            const std::uint32_t id = static_cast<std::uint32_t>(wmo.first);
            const std::uint16_t doodadSet = static_cast<std::uint16_t>(wmo.second->DoodadSet);

            stream.write(reinterpret_cast<const char *>(&id), sizeof(id));
            stream.write(reinterpret_cast<const char *>(&doodadSet), sizeof(doodadSet));
            stream << wmo.second->TransformMatrix;
            stream << wmo.second->Bounds;
            stream << std::left << std::setw(64) << std::setfill('\000') << wmo.second->Model->FileName;
        }
    }

    {
        std::lock_guard<std::mutex> guard(m_doodadMutex);

        const std::uint32_t doodadInstanceCount = static_cast<std::uint32_t>(m_loadedDoodadInstances.size());
        stream.write(reinterpret_cast<const char *>(&doodadInstanceCount), sizeof(doodadInstanceCount));

        for (auto const &doodad : m_loadedDoodadInstances)
        {
            const std::uint32_t id = static_cast<std::uint32_t>(doodad.first);
            stream.write(reinterpret_cast<const char *>(&id), sizeof(id));
            stream << doodad.second->TransformMatrix;
            stream << doodad.second->Bounds;
            stream << std::left << std::setw(64) << std::setfill('\000') << doodad.second->Model->FileName;
        }
    }
}
}