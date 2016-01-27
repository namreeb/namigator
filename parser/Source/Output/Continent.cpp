#include "parser.hpp"
#include "Input/Wdt/WdtFile.hpp"
#include "Output/Continent.hpp"
#include "Output/Adt.hpp"
#include "utility/Include/Directory.hpp"

#include <memory>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cassert>

namespace parser
{
Continent::Continent(const std::string &continentName) : Name(continentName), m_globalWmo(nullptr)
{
    std::string file = "World\\Maps\\" + Name + "\\" + Name + ".wdt";

    input::WdtFile continent(file);

    memcpy(m_hasAdt, continent.HasAdt, sizeof(bool) * 64 * 64);

    m_hasTerrain = continent.HasTerrain;

    if (!m_hasTerrain)
        m_globalWmo.reset(new Wmo(continent.Wmo->Vertices, continent.Wmo->Indices,
                                  continent.Wmo->LiquidVertices, continent.Wmo->LiquidIndices,
                                  continent.Wmo->DoodadVertices, continent.Wmo->DoodadIndices,
                                  continent.Wmo->Bounds));
}

const Adt *Continent::LoadAdt(int x, int y)
{
    std::lock_guard<std::mutex> guard(m_adtMutex);

    if (!m_hasAdt[y][x])
        return nullptr;

    if (!m_adts[y][x])
        m_adts[y][x].reset(new Adt(this, x, y));

    return m_adts[y][x].get();
}

void Continent::UnloadAdt(int x, int y)
{
    std::lock_guard<std::mutex> guard(m_adtMutex);
    m_adts[y][x].reset(nullptr);
}

bool Continent::HasAdt(int x, int y) const
{
    return m_hasAdt[y][x];
}

bool Continent::IsAdtLoaded(int x, int y) const
{
    std::lock_guard<std::mutex> guard(m_adtMutex);
    return !!m_adts[y][x];
}

bool Continent::IsWmoLoaded(unsigned int uniqueId) const
{
    std::lock_guard<std::mutex> guard(m_wmoMutex);
    return m_loadedWmos.find(uniqueId) != m_loadedWmos.end();
}

void Continent::InsertWmo(unsigned int uniqueId, const Wmo *wmo)
{
    assert(!!wmo);

    std::lock_guard<std::mutex> guard(m_wmoMutex);

    if (m_loadedWmos[uniqueId])
    {
        delete wmo;
#ifdef _DEBUG
        std::stringstream str;
        str << "Thread #" << std::setfill(' ') << std::setw(6) << std::this_thread::get_id()
            << " loaded already present wmo #" << uniqueId << ".  De-allocating and aborting load.\n";
        std::cout << str.str();
#endif
        return;
    }

    m_loadedWmos[uniqueId].reset(wmo);
#ifdef _DEBUG
    std::stringstream str;
    str << "Thread #" << std::setfill(' ') << std::setw(6) << std::this_thread::get_id()
        << " loaded WMO #" << uniqueId << ".  Needed by ADTs:";

    for (auto const &adt : wmo->Adts)
        str << " (" << adt.first << ", " << adt.second << "),";

    str << "\n";

    std::cout << str.str();
#endif
}

bool Continent::IsDoodadLoaded(unsigned int uniqueId) const
{
    std::lock_guard<std::mutex> guard(m_doodadMutex);
    return m_loadedDoodads.find(uniqueId) != m_loadedDoodads.end();
}

void Continent::InsertDoodad(unsigned int uniqueId, Doodad *doodad)
{
    std::lock_guard<std::mutex> guard(m_doodadMutex);
    m_loadedDoodads[uniqueId].reset(doodad);
}

const Wmo *Continent::GetGlobalWmo() const
{
    return m_globalWmo.get();
}

const Wmo *Continent::GetWmo(unsigned int uniqueId) const
{
    std::lock_guard<std::mutex> guard(m_wmoMutex);

    auto const itr = m_loadedWmos.find(uniqueId);

    return itr == m_loadedWmos.end() ? nullptr : itr->second.get();
}

void Continent::UnloadWmo(unsigned int uniqueId)
{
    std::lock_guard<std::mutex> guard(m_wmoMutex);
    m_loadedWmos.erase(uniqueId);
}

const Doodad *Continent::GetDoodad(unsigned int uniqueId) const
{
    std::lock_guard<std::mutex> guard(m_doodadMutex);

    auto const itr = m_loadedDoodads.find(uniqueId);

    return itr == m_loadedDoodads.end() ? nullptr : itr->second.get();
}

void Continent::UnloadDoodad(unsigned int uniqueId)
{
    std::lock_guard<std::mutex> guard(m_doodadMutex);
    m_loadedDoodads.erase(uniqueId);
}

#ifdef _DEBUG
void Continent::WriteMemoryUsage(std::ostream &o) const
{
    {
        std::lock_guard<std::mutex> guard(m_wmoMutex);
        o << "WMOs: " << m_loadedWmos.size() << std::endl;

        std::vector<std::pair<const Wmo *, unsigned int>> wmoMemoryUsage;
        wmoMemoryUsage.reserve(m_loadedWmos.size());

        for (auto i = m_loadedWmos.cbegin(); i != m_loadedWmos.cend(); i++)
        {
            const Wmo *wmo = i->second.get();

            wmoMemoryUsage.push_back({ wmo, wmo->MemoryUsage() });
        }

        std::sort(wmoMemoryUsage.begin(), wmoMemoryUsage.end(), [](const std::pair<const Wmo *, unsigned int> &a, const std::pair<const Wmo *, unsigned int> &b) { return a.second > b.second; });

        for (auto const &wmoUsage : wmoMemoryUsage)
            o << "0x" << std::hex << std::uppercase << reinterpret_cast<unsigned long>(wmoUsage.first) << ": " << std::dec << wmoUsage.second << std::endl;
    }

    {
        std::lock_guard<std::mutex> guard(m_doodadMutex);
        o << "Doodads: " << m_loadedDoodads.size() << std::endl;

        std::vector<std::pair<const Doodad *, unsigned int>> doodadMemoryUsage;
        doodadMemoryUsage.reserve(m_loadedDoodads.size());

        for (auto i = m_loadedDoodads.cbegin(); i != m_loadedDoodads.cend(); i++)
        {
            const Doodad *doodad = i->second.get();

            doodadMemoryUsage.push_back({ doodad, 0 });
        }

        std::sort(doodadMemoryUsage.begin(), doodadMemoryUsage.end(), [](const std::pair<const Doodad *, unsigned int> &a, const std::pair<const Doodad *, unsigned int> &b) { return a.second > b.second; });

        for (auto const &doodadUsage : doodadMemoryUsage)
            o << "0x" << std::hex << std::uppercase << reinterpret_cast<unsigned long>(doodadUsage.first) << ": " << std::dec << doodadUsage.second << std::endl;
    }

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