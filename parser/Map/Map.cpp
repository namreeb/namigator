#include "Map/Map.hpp"

#include "Adt/Adt.hpp"
#include "Common.hpp"
#include "DBC.hpp"
#include "MpqManager.hpp"
#include "Wmo/Wmo.hpp"
#include "Wmo/WmoPlacement.hpp"
#include "utility/BinaryStream.hpp"
#include "utility/Exception.hpp"
#include "utility/String.hpp"
#include "utility/Vector.hpp"

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>

namespace parser
{
Map::Map(const std::string& name)
    : m_globalWmo(nullptr), Name(name), Id(sMpqManager.GetMapId(name))
{
    auto const file = "World\\Maps\\" + Name + "\\" + Name + ".wdt";

    auto reader = sMpqManager.OpenFile(file);

    if (!reader)
        THROW(Result::WDT_OPEN_FAILED);

    size_t mphdLocation;
    if (!reader->GetChunkLocation("MPHD", mphdLocation))
        THROW(Result::MPHD_NOT_FOUND);

    reader->rpos(mphdLocation + 8);

    auto const firstArg = reader->Read<std::uint32_t>();

    size_t mainLocation;
    if (!reader->GetChunkLocation("MAIN", mainLocation))
        THROW(Result::MAIN_NOT_FOUND);

    reader->rpos(mainLocation + 4);
    auto const mainSize = reader->Read<std::uint32_t>();

    constexpr size_t alphaMainSize = 64 * 64 * 4 * sizeof(std::uint32_t);

    // at this point, the reader is pointing to the start of the MAIN data

    // alpha data will have a predictable size for this struct, and it will
    // differ from other versions.  so we use that value to check if we are
    // reading alpha data or not.
    m_isAlphaData = mainSize == alphaMainSize;

    std::string globalWmoName;

    if (m_isAlphaData)
    {
        // save this data in memory for use by worker threads
        // TODO: transfer ownership from reader to m_alphaData, replacing
        // reader with a weak pointer?
        reader->rpos(0);
        m_alphaData =
            std::make_shared<std::vector<std::uint8_t>>(reader->wpos());
        reader->ReadBytes(&m_alphaData->at(0), m_alphaData->size());

        // go back and read doodad and wmo names
        reader->rpos(mphdLocation + 8);
        auto const doodadNameCount = reader->Read<std::uint32_t>();
        auto const doodadNameOffset = reader->Read<std::uint32_t>();
        auto const wmoNameCount = reader->Read<std::uint32_t>();
        auto const wmoNameOffset = reader->Read<std::uint32_t>();

        m_doodadNames.reserve(doodadNameCount);
        m_wmoNames.reserve(wmoNameCount);

        // restore pointer to MAIN
        reader->rpos(mainLocation + 8);

        for (int y = 0; y < MeshSettings::Adts; ++y)
            for (int x = 0; x < MeshSettings::Adts; ++x)
            {
                m_adtOffsets[x][y] = reader->Read<std::uint32_t>();
                auto const size = reader->Read<std::uint32_t>();

                m_hasAdt[x][y] = size != 0;

                reader->rpos(reader->rpos() + 8);
            }

        size_t mdnmLocation;
        if (!reader->GetChunkLocation("MDNM", doodadNameOffset, mdnmLocation))
            THROW(Result::MDNM_NOT_FOUND);

        reader->rpos(mdnmLocation + 4);
        auto const mdnmSize = reader->Read<std::uint32_t>();
        char* p;
        // Can be 0.
        if (mdnmSize > 0)
        {

            std::vector<char> doodadNameBuffer(mdnmSize);
            reader->ReadBytes(&doodadNameBuffer[0], doodadNameBuffer.size());

            for (p = &doodadNameBuffer[0]; *p == '\0'; ++p)
                ;

            do
            {
                m_doodadNames.emplace_back(p);
                p = strchr(p, '\0');
                if (!p)
                    break;
                ++p;
            } while (p <= &doodadNameBuffer[doodadNameBuffer.size() - 1]);
        }

        size_t monmLocation;
        if (!reader->GetChunkLocation("MONM", wmoNameOffset, monmLocation))
            THROW(Result::MONM_NOT_FOUND);

        reader->rpos(monmLocation + 4);
        auto const monmSize = reader->Read<std::uint32_t>();

        // Truncated files, i.e. UnderMine.
        if (monmSize == 0 || reader->IsEOF())
            THROW(Result::BUFFER_TOO_SMALL);

        std::vector<char> wmoNameBuffer(monmSize);
        reader->ReadBytes(&wmoNameBuffer[0], wmoNameBuffer.size());

        for (p = &wmoNameBuffer[0]; *p == '\0'; ++p)
            ;

        do
        {
            m_wmoNames.emplace_back(p);
            p = strchr(p, '\0');
            if (!p)
                break;
            ++p;
        } while (p <= &wmoNameBuffer[wmoNameBuffer.size() - 1]);
    }
    else
    {
        m_alphaData.reset();
        m_hasTerrain = !(firstArg & 0x1);

        for (int y = 0; y < MeshSettings::Adts; ++y)
            for (int x = 0; x < MeshSettings::Adts; ++x)
            {
                auto const flag = reader->Read<std::uint32_t>();

                // skip async object
                reader->rpos(reader->rpos() + 4);

                m_hasAdt[x][y] = !!(flag & 1);
            }

        // for worlds with terrain, parsing stops here.  else, load single wmo
        if (m_hasTerrain)
            return;

        size_t mwmoLocation;
        if (!reader->GetChunkLocation("MWMO", mwmoLocation))
            THROW(Result::MWMO_NOT_FOUND);
        reader->rpos(mwmoLocation + 8);

        globalWmoName = reader->ReadString();
    }

    size_t modfLocation;
    // the alpha client stores the global WMO data in a specific location.
    // we cannot use our fault tolerant chunk searching code here because in
    // the same file there are MODF chunks within the ADT data.  so we manually
    // check the next chunk after MONM, which should be the last thing we read.
    if (m_isAlphaData)
    {
        auto const old = reader->rpos();
        auto const chunk = reader->Read<std::uint32_t>();
        reader->rpos(old);

        if (chunk == 'MODF')
        {
            modfLocation = old;
            m_hasTerrain = false;
        }
        else
        {
            m_hasTerrain = true;
            return;
        }
    }
    else if (!reader->GetChunkLocation("MODF", modfLocation))
        THROW(Result::MODF_NOT_FOUND);

    reader->rpos(modfLocation + 4);
    auto const modfSize = reader->Read<std::uint32_t>();

    input::WmoPlacement placement;
    reader->ReadBytes(&placement, sizeof(placement));

    if (m_isAlphaData)
        globalWmoName = m_wmoNames[placement.NameId];

    math::BoundingBox bounds;
    placement.GetBoundingBox(bounds);

    math::Matrix transformMatrix;
    placement.GetTransformMatrix(transformMatrix);

    m_globalWmo = std::make_unique<WmoInstance>(
        GetWmo(globalWmoName), placement.DoodadSet, placement.NameSet, bounds,
        transformMatrix);
}

bool Map::HasAdt(int x, int y) const
{
    assert(x >= 0 && y >= 0 && x < MeshSettings::Adts &&
           y < MeshSettings::Adts);

    return m_hasAdt[x][y];
}

const Adt* Map::GetAdt(int x, int y)
{
    assert(x >= 0 && y >= 0 && x < MeshSettings::Adts &&
           y < MeshSettings::Adts);

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

const Wmo* Map::GetWmo(const std::string& name)
{
    auto filename = utility::lower(name);

    std::lock_guard<std::mutex> guard(m_wmoMutex);

    for (auto const& wmo : m_loadedWmos)
        if (wmo->MpqPath == filename)
            return wmo.get();

    auto ret = new Wmo(filename);

    m_loadedWmos.push_back(std::unique_ptr<const Wmo>(ret));

    // loading this WMO also loaded all referenced doodads in all doodad sets
    // for the wmo. for now, let us share ownership between the WMO and this map
    std::lock_guard<std::mutex> doodadGuard(m_doodadMutex);
    for (auto const& doodadSet : ret->DoodadSets)
        for (auto const& wmoDoodad : doodadSet)
        {
            bool present = false;
            for (auto const& d : m_loadedDoodads)
                if (d->MpqPath == wmoDoodad->Parent->MpqPath)
                {
                    present = true;
                    break;
                }

            if (!present)
                m_loadedDoodads.push_back(wmoDoodad->Parent);
        }
    return ret;
}

void Map::InsertWmoInstance(unsigned int uniqueId, const WmoInstance* wmo)
{
    std::lock_guard<std::mutex> guard(m_wmoMutex);
    m_loadedWmoInstances[uniqueId].reset(wmo);
}

const WmoInstance* Map::GetWmoInstance(unsigned int uniqueId) const
{
    std::lock_guard<std::mutex> guard(m_wmoMutex);

    auto const itr = m_loadedWmoInstances.find(uniqueId);

    return itr == m_loadedWmoInstances.end() ? nullptr : itr->second.get();
}

const WmoInstance* Map::GetGlobalWmoInstance() const
{
    return m_globalWmo.get();
}

const Doodad* Map::GetDoodad(const std::string& name)
{
    auto filename = utility::lower(name);

    std::lock_guard<std::mutex> guard(m_doodadMutex);

    for (auto const& doodad : m_loadedDoodads)
        if (doodad->MpqPath == filename)
            return doodad.get();

    auto ret = new Doodad(name);
    m_loadedDoodads.push_back(std::unique_ptr<const Doodad>(ret));
    return ret;
}

void Map::InsertDoodadInstance(unsigned int uniqueId,
                               const DoodadInstance* doodad)
{
    std::lock_guard<std::mutex> guard(m_doodadMutex);
    m_loadedDoodadInstances[uniqueId].reset(doodad);
}

const DoodadInstance* Map::GetDoodadInstance(unsigned int uniqueId) const
{
    std::lock_guard<std::mutex> guard(m_doodadMutex);

    auto const itr = m_loadedDoodadInstances.find(uniqueId);

    return itr == m_loadedDoodadInstances.end() ? nullptr : itr->second.get();
}

void Map::Serialize(utility::BinaryStream& stream) const
{
    const size_t ourSize =
        sizeof(std::uint32_t) + sizeof(std::uint8_t) +
        (m_hasTerrain ?
             (sizeof(std::uint8_t) * ( // has_adt map compressed into bitfield
                                         sizeof(m_hasAdt) / 8) +
              sizeof(std::uint32_t) +  // loaded wmo size
              (sizeof(std::uint32_t) + // id
               sizeof(std::uint16_t) + // doodad set
               sizeof(std::uint16_t) + // name set
               22 * sizeof(float) + // 16 floats for transform matrix, 6 floats
                                    // for bounds
               MeshSettings::MaxMPQPathLength    // model file name
               ) * m_loadedWmoInstances.size() + // for each wmo instance
              sizeof(std::uint32_t) +            // loaded doodad size
              (sizeof(std::uint32_t) +           // id
               22 * sizeof(float) + // 16 floats for transform matrix, 6 floats
                                    // for bounds
               MeshSettings::MaxMPQPathLength // model file name
               ) * m_loadedDoodadInstances.size()) :
             (sizeof(std::uint32_t) + // id
              sizeof(std::uint16_t) + // doodad set
              sizeof(std::uint16_t) + // name set
              22 * sizeof(float) + // 16 floats for transform matrix, 6 floats
                                   // for bounds
              MeshSettings::MaxMPQPathLength // model file name
              ));

    utility::BinaryStream ourStream(ourSize);
    ourStream << MeshSettings::FileMap;

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

            ourStream << static_cast<std::uint32_t>(
                m_loadedWmoInstances.size());

            for (auto const& wmo : m_loadedWmoInstances)
            {
                ourStream << static_cast<std::uint32_t>(wmo.first);
                ourStream << static_cast<std::uint16_t>(wmo.second->DoodadSet);
                ourStream << static_cast<std::uint16_t>(wmo.second->NameSet);
                ourStream << wmo.second->TransformMatrix;
                ourStream << wmo.second->Bounds;
                ourStream.WriteString(wmo.second->Model->MpqPath,
                                      MeshSettings::MaxMPQPathLength);
            }
        }

        {
            std::lock_guard<std::mutex> guard(m_doodadMutex);

            ourStream << static_cast<std::uint32_t>(
                m_loadedDoodadInstances.size());

            for (auto const& doodad : m_loadedDoodadInstances)
            {
                ourStream << static_cast<std::uint32_t>(doodad.first);
                ourStream << doodad.second->TransformMatrix;
                ourStream << doodad.second->Bounds;
                ourStream.WriteString(doodad.second->Model->MpqPath,
                                      MeshSettings::MaxMPQPathLength);
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
        ourStream << static_cast<std::uint16_t>(wmo->NameSet);
        ourStream << wmo->TransformMatrix;
        ourStream << wmo->Bounds;
        ourStream.WriteString(wmo->Model->MpqPath,
                              MeshSettings::MaxMPQPathLength);
    }

    // make sure our prediction of the final size is correct, to avoid
    // reallocations
    assert(ourSize == ourStream.wpos());

    stream << ourStream;
}
} // namespace parser