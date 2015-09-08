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

    Adt *Continent::LoadAdt(int x, int y)
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

    void Continent::SaveToDisk(bool writeAdts)
    {
        const char zero = 0;
        std::stringstream ss;

        if (!Directory::Exists("Data"))
            Directory::Create("Data");

        if (!Directory::Exists(std::string("Data\\") + Name))
            Directory::Create(std::string("Data\\") + Name);

        ss << "Data\\" << Name << "\\" << Name << ".ncon";

        std::ofstream out(ss.str(), std::ios::out | std::ios::binary);

        out.write(VERSION, strlen(VERSION));
        out.write(&zero, 1);
        out.write("CON", 3);

        if (!m_hasTerrain)
        {
            // write HasTerrain = false
            out.write(&zero, 1);

            std::stringstream wmoName;

            wmoName << "Data\\" << Name << "\\" << Name << ".nwmo";

            m_wmo->SaveToDisk(wmoName.str());

            return;
        }

        // write HasTerrain = true
        const char one = 1;
        out.write(&one, 1);

        out.write((const char *)&m_hasAdt, sizeof(bool)*64*64);

        const unsigned int wmoCount = m_savedWmos.size() + m_loadedWmos.size();
        out.write((const char *)&wmoCount, sizeof(unsigned int));

        if (wmoCount > 0 && !Directory::Exists(std::string("Data\\") + Name + std::string("\\WMOs")))
                Directory::Create(std::string("Data\\") + Name + std::string("\\WMOs"));

        const unsigned int doodadCount = m_savedDoodads.size() + m_loadedDoodads.size();
        out.write((const char *)&doodadCount, sizeof(unsigned int));

        if (doodadCount > 0 && !Directory::Exists(std::string("Data\\") + Name + std::string("\\Doodads")))
                Directory::Create(std::string("Data\\") + Name + std::string("\\Doodads"));

        for (auto i = m_savedWmos.begin(); i != m_savedWmos.end(); ++i)
        {
            const unsigned int wmoId = *i;
            out.write((const char *)&wmoId, sizeof(unsigned int));
        }

        for (auto i = m_loadedWmos.begin(); i != m_loadedWmos.end(); ++i)
        {
            const unsigned int wmoId = i->first;
            auto wmo = i->second.get();

            out.write((const char *)&wmoId, sizeof(unsigned int));

            std::stringstream wmoName;

            wmoName << "Data\\" << Name << "\\WMOs\\" << wmoId << ".nwmo";

            wmo->SaveToDisk(wmoName.str());

            m_savedWmos.insert(wmoId);
        }

        m_loadedWmos.clear();

        for (auto i = m_savedDoodads.begin(); i != m_savedDoodads.end(); ++i)
        {
            const unsigned int doodadId = *i;
            out.write((const char *)&doodadId, sizeof(unsigned int));
        }

        for (auto i = m_loadedDoodads.begin(); i != m_loadedDoodads.end(); ++i)
        {
            const unsigned int doodadId = i->first;
            auto doodad = i->second.get();

            out.write((const char *)&doodadId, sizeof(unsigned int));

            std::stringstream doodadName;

            doodadName << "Data\\" << Name << "\\Doodads\\" << doodadId << ".ndoo";

            doodad->SaveToDisk(doodadName.str());

            m_savedDoodads.insert(doodadId);
        }

        m_loadedDoodads.clear();

        out.close();

        if (writeAdts)
            for (auto i = m_adts.begin(); i != m_adts.end(); ++i)
                i->second->SaveToDisk();
    }
}