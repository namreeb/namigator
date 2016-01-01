#include <memory>
#include <sstream>
#include <fstream>

#include "Input/Wdt/WdtFile.hpp"
#include "Output/Continent.hpp"
#include "Output/Adt.hpp"
#include "parser.hpp"
#include "Directory.hpp"

using namespace parser_input;

#define CONV(x, y) (y * 64 + x)

namespace parser
{
    Continent::Continent(const std::string &continentName) : Name(continentName)
    {
        std::string file = "World\\Maps\\" + Name + "\\" + Name + ".wdt";

        WdtFile continent(file);

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
        if (m_adts.find(CONV(x, y)) == m_adts.end())
            m_adts[CONV(x, y)] = std::unique_ptr<Adt>(new Adt(this, x, y));

        return m_adts[CONV(x, y)].get();
    }

    bool Continent::IsAdtLoaded(int x, int y) const
    {
        return m_adts.find(CONV(x, y)) != m_adts.end();
    }

    bool Continent::IsWmoLoaded(unsigned int uniqueId) const
    {
        return m_loadedWmos.find(uniqueId) != m_loadedWmos.end();
    }

    bool Continent::IsDoodadLoaded(unsigned int uniqueId) const
    {
        return m_loadedDoodads.find(uniqueId) != m_loadedDoodads.end();
    }

    void Continent::InsertWmo(unsigned int uniqueId, Wmo *wmo)
    {
        m_loadedWmos[uniqueId].reset(wmo);
    }

    void Continent::InsertDoodad(unsigned int uniqueId, Doodad *doodad)
    {
        m_loadedDoodads[uniqueId].reset(doodad);
    }

    const Wmo *Continent::GetWmo(unsigned int uniqueId) const
    {
        return IsWmoLoaded(uniqueId) ? m_loadedWmos.at(uniqueId).get() : nullptr;
    }

    const Doodad *Continent::GetDoodad(unsigned int uniqueId) const
    {
        return IsDoodadLoaded(uniqueId) ? m_loadedDoodads.at(uniqueId).get() : nullptr;
    }
}