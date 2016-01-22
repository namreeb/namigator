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
}