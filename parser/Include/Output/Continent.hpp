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

            void Load();

            // hold wmos/doodads in memory
            std::map<unsigned int, std::unique_ptr<Wmo>> m_loadedWmos;
            std::map<unsigned int, std::unique_ptr<Doodad>> m_loadedDoodads;

            // when the continent is saved, we no longer need wmos/doodads in memory.
            // instead, we keep a list of the ones we've saved.  this way we can save
            // the continent again as we load more adts without resaving/reloading
            // wmos/doodads we have already encountered.
            std::set<unsigned int> m_savedWmos;
            std::set<unsigned int> m_savedDoodads;

            bool m_hasAdt[64][64];
            bool m_hasTerrain;

            std::map<int, std::unique_ptr<Adt>> m_adts;

            std::unique_ptr<Wmo> m_wmo;

        public:
            const std::string Name;

            Continent(const std::string &continentName);
            Continent(const char *continentName);

            Adt *LoadAdt(int x, int y);

            bool HasWmo(unsigned int uniqueId) const;
            bool HasDoodad(unsigned int uniqueId) const;

            bool HasWmoLoaded(unsigned int uniqueId) const;
            bool HasDoodadLoaded(unsigned int uniqueId) const;

            bool HasWmoSaved(unsigned int uniqueId) const;
            bool HasDoodadSaved(unsigned int uniqueId) const;

            void InsertWmo(unsigned int uniqueId, Wmo *wmo);
            void InsertDoodad(unsigned int uniqueId, Doodad *doodad);

            Wmo *GetWmo(unsigned int uniqueId) const;
            Doodad *GetDoodad(unsigned int uniqueId) const;

            void SaveToDisk(bool writeAdts = true);
    };
}