#include "MpqManager.hpp"
#include "DBC.hpp"
#include "Map/Map.hpp"
#include "Adt/Adt.hpp"
#include "Wmo/Wmo.hpp"
#include "Wmo/WmoPlacement.hpp"

#include "utility/BinaryStream.hpp"
#include "utility/Exception.hpp"
#include "utility/Vector.hpp"

#include "Common.hpp"

#include <memory>
#include <sstream>
#include <fstream>
#include <ostream>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <cstdint>

namespace
{
std::uint32_t GetRootAreaId(const std::unordered_map<std::uint32_t, std::uint32_t> &areas, std::uint32_t id)
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
}

namespace parser
{
Map::Map(const std::string &name) : m_globalWmo(nullptr), Name(name), Id(sMpqManager.GetMapId(name))
{
    auto const file = "World\\Maps\\" + Name + "\\" + Name + ".wdt";

    std::unique_ptr<utility::BinaryStream> reader(sMpqManager.OpenFile(file));

    if (!reader)
        THROW("WDT open failed");

    size_t mphdLocation;
    if (!reader->GetChunkLocation("MPHD", mphdLocation))
        THROW("MPHD not found");

    reader->rpos(mphdLocation + 8);

    m_hasTerrain = !(reader->Read<unsigned int>() & 0x1);

    size_t mainLocation;
    if (!reader->GetChunkLocation("MAIN", mainLocation))
        THROW("MAIN not found");

    reader->rpos(mainLocation + 8);

    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x)
        {
            auto const flag = reader->Read<int>();

            // skip async object
            reader->rpos(reader->rpos() + 4);

            m_hasAdt[x][y] = !!(flag & 1);
        }

    DBC area("DBFilesClient\\AreaTable.dbc");

    std::unordered_map<std::uint32_t, std::uint32_t> area_to_zone;

    for (auto i = 0; i < area.RecordCount(); ++i)
    {
        auto const map = area.GetField(i, 1);

        // we don't care about the areas not associated with this map
        if (map != Id)
            continue;

        auto const id = area.GetField(i, 0);
        auto const parent = area.GetField(i, 2);

        area_to_zone[id] = parent;
    }

    for (auto const &i : area_to_zone)
        m_areaToZone[i.first] = GetRootAreaId(area_to_zone, i.first);

    // for worlds with terrain, parsing stops here.  else, load single wmo
    if (m_hasTerrain)
        return;

    size_t mwmoLocation;
    if (!reader->GetChunkLocation("MWMO", mwmoLocation))
        THROW("MWMO not found");
    reader->rpos(mwmoLocation + 8);

    auto const wmoName = reader->ReadString();

    size_t modfLocation;
    if (!reader->GetChunkLocation("MODF", modfLocation))
        THROW("MODF not found");
    reader->rpos(modfLocation + 8);

    input::WmoPlacement placement;
    reader->ReadBytes(&placement, sizeof(placement));

    math::BoundingBox bounds;
    placement.GetBoundingBox(bounds);

    math::Matrix transformMatrix;
    placement.GetTransformMatrix(transformMatrix);

    m_globalWmo = std::make_unique<WmoInstance>(GetWmo(wmoName), placement.DoodadSet, bounds, transformMatrix);
}

std::uint32_t Map::ZoneFromArea(std::uint32_t areaId) const
{
    auto const i = m_areaToZone.find(areaId);

    if (i == m_areaToZone.end())
        THROW("Area ID not found");

    return i->second;
}

bool Map::HasAdt(int x, int y) const
{
    assert(x >= 0 && y >= 0 && x < MeshSettings::Adts && y < MeshSettings::Adts);

    return m_hasAdt[x][y];
}

const Adt *Map::GetAdt(int x, int y)
{
    assert(x >= 0 && y >= 0 && x < MeshSettings::Adts && y < MeshSettings::Adts);

    std::lock_guard<std::mutex> guard(m_adtMutex);

    if (!m_hasAdt[x][y])
        return nullptr;

    if (!m_adts[x][y])
        m_adts[x][y] = std::make_unique<Adt>(this, x, y);

    return m_adts[x][y].get();
}

void Map::UnloadAdt(int x, int y)
{
    std::lock_guard<std::mutex> guard(m_adtMutex);
    m_adts[x][y].reset();
}

const Wmo *Map::GetWmo(const std::string &name)
{
    auto filename = name.substr(name.rfind('\\') + 1);
    filename = filename.substr(0, filename.rfind('.'));

    std::lock_guard<std::mutex> guard(m_wmoMutex);
    
    for (auto const &wmo : m_loadedWmos)
        if (wmo->MpqPath == filename)
            return wmo.get();

    auto ret = new Wmo(name);

    m_loadedWmos.push_back(std::unique_ptr<const Wmo>(ret));

    // loading this WMO also loaded all referenced doodads in all doodad sets for the wmo.
    // for now, let us share ownership between the WMO and this map
    for (auto const &doodadSet : ret->DoodadSets)
        for (auto const &wmoDoodad : doodadSet)
            m_loadedDoodads.push_back(wmoDoodad->Parent);

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
        if (doodad->MpqPath == filename)
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

void Map::Serialize(utility::BinaryStream& stream) const
{
    const size_t ourSize = sizeof(std::uint32_t) + sizeof(std::uint8_t) +
        (m_hasTerrain ?
            (
                sizeof(std::uint8_t) * (            // has_adt map compressed into bitfield
                    sizeof(m_hasAdt) / 8
                ) +
                sizeof(std::uint32_t) +             // loaded wmo size
                (
                    sizeof(std::uint32_t) +         // id
                    sizeof(std::uint16_t) +         // doodad set
                    22 * sizeof(float) +            // 16 floats for transform matrix, 6 floats for bounds
                    MeshSettings::MaxMPQPathLength  // model file name
                ) * m_loadedWmoInstances.size() +   // for each wmo instance
                sizeof(std::uint32_t) +             // loaded doodad size
                (
                    sizeof(std::uint32_t) +         // id
                    22 * sizeof(float) +            // 16 floats for transform matrix, 6 floats for bounds
                    MeshSettings::MaxMPQPathLength  // model file name
                ) * m_loadedDoodadInstances.size()
            )
        :
            (
                sizeof(std::uint32_t) +             // id
                sizeof(std::uint16_t) +             // doodad set
                22 * sizeof(float) +                // 16 floats for transform matrix, 6 floats for bounds
                MeshSettings::MaxMPQPathLength      // model file name
            ));

    utility::BinaryStream ourStream(ourSize);
    ourStream << Magic;

    static_assert(sizeof(bool) == 1, "bool must be only 1 byte");

    if (m_hasTerrain)
    {
        ourStream << (std::uint8_t)1;

        std::uint8_t has_adt[sizeof(m_hasAdt) / 8];
        ::memset(has_adt, 0, sizeof(has_adt));

        for (auto y = 0; y < MeshSettings::Adts; ++y)
            for (auto x = 0; x < MeshSettings::Adts; ++x)
            {
                if (!m_hasAdt[x][y])
                    continue;

                auto const offset = y * MeshSettings::Adts + x;
                auto const byte_offset = offset / 8;
                auto const bit_offset = offset % 8;

                has_adt[byte_offset] |= (1 << bit_offset);
            }

        ourStream.Write(has_adt, sizeof(has_adt));

        {
            std::lock_guard<std::mutex> guard(m_wmoMutex);

            ourStream << static_cast<std::uint32_t>(m_loadedWmoInstances.size());

            for (auto const &wmo : m_loadedWmoInstances)
            {
                ourStream << static_cast<std::uint32_t>(wmo.first);
                ourStream << static_cast<std::uint16_t>(wmo.second->DoodadSet);
                ourStream << wmo.second->TransformMatrix;
                ourStream << wmo.second->Bounds;
                ourStream.WriteString(wmo.second->Model->MpqPath, MeshSettings::MaxMPQPathLength);
            }
        }

        {
            std::lock_guard<std::mutex> guard(m_doodadMutex);

            ourStream << static_cast<std::uint32_t>(m_loadedDoodadInstances.size());

            for (auto const &doodad : m_loadedDoodadInstances)
            {
                ourStream << static_cast<std::uint32_t>(doodad.first);
                ourStream << doodad.second->TransformMatrix;
                ourStream << doodad.second->Bounds;
                ourStream.WriteString(doodad.second->Model->MpqPath, MeshSettings::MaxMPQPathLength);
            }
        }
    }
    else
    {
        ourStream << (std::uint8_t)0;

        auto const wmo = GetGlobalWmoInstance();

        constexpr std::uint32_t id = 0xFFFFFFFF;

        ourStream << id;
        ourStream << static_cast<std::uint16_t>(wmo->DoodadSet);
        ourStream << wmo->TransformMatrix;
        ourStream << wmo->Bounds;
        ourStream.WriteString(wmo->Model->MpqPath, MeshSettings::MaxMPQPathLength);
    }

    // make sure our prediction of the final size is correct, to avoid reallocations
    assert(ourSize == ourStream.wpos());

    stream << ourStream;
}
}