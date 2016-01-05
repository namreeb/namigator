#include "parser.hpp"
#include "Input/Wdt/WdtFile.hpp"
#include "Output/Continent.hpp"
#include "Output/Adt.hpp"
#include "utility/Include/Directory.hpp"

#include <memory>
#include <sstream>
#include <fstream>

#define CONV(x, y) (y * 64 + x)

namespace parser
{
Continent::Continent(const std::string &continentName) : Name(continentName)
{
    std::string file = "World\\Maps\\" + Name + "\\" + Name + ".wdt";

    input::WdtFile continent(file);

    memcpy(m_hasAdt, continent.HasAdt, sizeof(bool) * 64 * 64);

    m_hasTerrain = continent.HasTerrain;

    if (!m_hasTerrain)
        m_wmo.reset(new Wmo(continent.Wmo->Vertices, continent.Wmo->Indices,
                            continent.Wmo->LiquidVertices, continent.Wmo->LiquidIndices,
                            continent.Wmo->DoodadVertices, continent.Wmo->DoodadIndices,
                            continent.Wmo->Bounds.MinCorner.Z, continent.Wmo->Bounds.MaxCorner.Z));
}

const Adt *Continent::LoadAdt(int x, int y)
{
    std::lock_guard<std::mutex> guard(m_adtMutex);

    if (m_adts.find(CONV(x, y)) == m_adts.end())
        m_adts[CONV(x, y)] = std::unique_ptr<Adt>(new Adt(this, x, y));

    return m_adts[CONV(x, y)].get();
}

bool Continent::IsAdtLoaded(int x, int y) const
{
    std::lock_guard<std::mutex> guard(m_adtMutex);

    return m_adts.find(CONV(x, y)) != m_adts.end();
}

bool Continent::IsWmoLoaded(unsigned int uniqueId) const
{
    std::lock_guard<std::mutex> guard(m_wmoMutex);

    return m_loadedWmos.find(uniqueId) != m_loadedWmos.end();
}

bool Continent::IsDoodadLoaded(unsigned int uniqueId) const
{
    std::lock_guard<std::mutex> guard(m_doodadMutex);

    return m_loadedDoodads.find(uniqueId) != m_loadedDoodads.end();
}

void Continent::InsertWmo(unsigned int uniqueId, Wmo *wmo)
{
    std::lock_guard<std::mutex> guard(m_wmoMutex);

    m_loadedWmos[uniqueId].reset(wmo);
}

void Continent::InsertDoodad(unsigned int uniqueId, Doodad *doodad)
{
    std::lock_guard<std::mutex> guard(m_doodadMutex);

    m_loadedDoodads[uniqueId].reset(doodad);
}

const Wmo *Continent::GetWmo() const
{
    return m_wmo.get();
}

const Wmo *Continent::GetWmo(unsigned int uniqueId) const
{
    std::lock_guard<std::mutex> guard(m_wmoMutex);

    auto const itr = m_loadedWmos.find(uniqueId);

    return itr == m_loadedWmos.end() ? nullptr : itr->second.get();
}

const Doodad *Continent::GetDoodad(unsigned int uniqueId) const
{
    std::lock_guard<std::mutex> guard(m_doodadMutex);

    auto const itr = m_loadedDoodads.find(uniqueId);

    return itr == m_loadedDoodads.end() ? nullptr : itr->second.get();
}
}