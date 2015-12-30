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
    void Continent::Load()
    {
        std::string file = "World\\Maps\\" + Name + "\\" + Name + ".wdt";

        WdtFile continent(file);

        memcpy(m_hasAdt, continent.HasAdt, sizeof(bool)*64*64);

        m_hasTerrain = continent.HasTerrain;

        if (!m_hasTerrain)
            m_wmo = std::unique_ptr<Wmo>(new Wmo(continent.Wmo->Vertices, continent.Wmo->Indices,
                                                 continent.Wmo->LiquidVertices, continent.Wmo->LiquidIndices,
                                                 continent.Wmo->DoodadVertices, continent.Wmo->DoodadIndices,
                                                 continent.Wmo->Bounds.MinCorner.Z, continent.Wmo->Bounds.MaxCorner.Z));
    }

    Continent::Continent(const std::string &continentName) : Name(continentName)
    {
        Load();
    }

    Continent::Continent(const char *continentName) : Name(continentName)
    {
        Load();
    }

    const Adt *Continent::LoadAdt(int x, int y)
    {
        if (m_adts.find(CONV(x, y)) == m_adts.end())
            m_adts[CONV(x, y)] = std::unique_ptr<Adt>(new Adt(this, x, y));

        return m_adts[CONV(x, y)].get();
    }

    bool Continent::HasWmo(unsigned int uniqueId) const
    {
        return HasWmoLoaded(uniqueId) || HasWmoSaved(uniqueId);
    }

    bool Continent::HasDoodad(unsigned int uniqueId) const
    {
        return HasDoodadLoaded(uniqueId) || HasDoodadSaved(uniqueId);
    }

    bool Continent::HasWmoLoaded(unsigned int uniqueId) const
    {
        return m_loadedWmos.find(uniqueId) != m_loadedWmos.end();
    }

    bool Continent::HasDoodadLoaded(unsigned int uniqueId) const
    {
        return m_loadedDoodads.find(uniqueId) != m_loadedDoodads.end();
    }

    bool Continent::HasWmoSaved(unsigned int uniqueId) const
    {
        return m_savedWmos.find(uniqueId) != m_savedWmos.end();
    }

    bool Continent::HasDoodadSaved(unsigned int uniqueId) const
    {
        return m_savedDoodads.find(uniqueId) != m_savedDoodads.end();
    }

    void Continent::InsertWmo(unsigned int uniqueId, Wmo *wmo)
    {
        if (HasWmo(uniqueId))
            return;

        m_loadedWmos[uniqueId].reset(wmo);
    }

    void Continent::InsertDoodad(unsigned int uniqueId, Doodad *doodad)
    {
        if (HasDoodad(uniqueId))
            return;

        m_loadedDoodads[uniqueId].reset(doodad);
    }

    Wmo *Continent::GetWmo(unsigned int uniqueId) const
    {
        return HasWmoLoaded(uniqueId) ? m_loadedWmos.at(uniqueId).get() : nullptr;
    }

    Doodad *Continent::GetDoodad(unsigned int uniqueId) const
    {
        return HasDoodadLoaded(uniqueId) ? m_loadedDoodads.at(uniqueId).get() : nullptr;
    }
}