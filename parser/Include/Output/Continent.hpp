#pragma once

#include <memory>
#include <string>
#include <map>

#include "Output/Wmo.hpp"
#include "Output/Doodad.hpp"
#include "Output/Adt.hpp"

namespace parser
{
    class Continent
    {
        private:
            //ContinentIdMap["Azeroth"] = 0;
            //ContinentIdMap["Kalimdor"] = 1;
            //ContinentIdMap["PVPZone01"] = 30;
            //ContinentIdMap["DeadminesInstance"] = 36;
            //ContinentIdMap["PVPZone03"] = 489;
            //ContinentIdMap["PVPZone04"] = 529;
            //ContinentIdMap["Expansion01"] = 530;
            //ContinentIdMap["Northrend"] = 571;

            //ContinentInternalName["Azeroth"] = "Azeroth";
            //ContinentInternalName["Kalimdor"] = "Kalimdor";
            //ContinentInternalName["Alterac Valley"] = "PVPZone01";
            //ContinentInternalName["Deadmines"] = "DeadminesInstance";
            //ContinentInternalName["Warsong Gulch"] = "PVPZone03";
            //ContinentInternalName["Arathi Basin"] = "PVPZone04";
            //ContinentInternalName["Outland"] = "Expansion01";
            //ContinentInternalName["Northrend"] = "Northrend";

            // hold wmos/doodads in memory
            std::map<unsigned int, std::unique_ptr<Wmo>> m_loadedWmos;
            std::map<unsigned int, std::unique_ptr<Doodad>> m_loadedDoodads;

            bool m_hasAdt[64][64];
            bool m_hasTerrain;

            std::map<int, std::unique_ptr<Adt>> m_adts;

            std::unique_ptr<Wmo> m_wmo;

        public:
            const std::string Name;

            Continent(const std::string &continentName);

            const Adt *LoadAdt(int x, int y);

            bool IsAdtLoaded(int x, int y) const;
            bool IsWmoLoaded(unsigned int uniqueId) const;
            bool IsDoodadLoaded(unsigned int uniqueId) const;

            void InsertWmo(unsigned int uniqueId, Wmo *wmo);
            void InsertDoodad(unsigned int uniqueId, Doodad *doodad);

            const Wmo *GetWmo(unsigned int uniqueId) const;
            const Doodad *GetDoodad(unsigned int uniqueId) const;
    };
}