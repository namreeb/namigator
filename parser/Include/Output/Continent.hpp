#pragma once

#include "Output/Wmo.hpp"
#include "Output/Doodad.hpp"
#include "Output/Adt.hpp"

#include <mutex>
#include <memory>
#include <string>
#include <map>

namespace parser
{
    class Continent
    {
        private:
            mutable std::mutex m_wmoMutex;
            mutable std::mutex m_doodadMutex;

            std::map<unsigned int, std::unique_ptr<Wmo>> m_loadedWmos;
            std::map<unsigned int, std::unique_ptr<Doodad>> m_loadedDoodads;

            bool m_hasAdt[64][64];
            bool m_hasTerrain;

            mutable std::mutex m_adtMutex;
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

            const Wmo *GetWmo() const;
            const Wmo *GetWmo(unsigned int uniqueId) const;
            const Doodad *GetDoodad(unsigned int uniqueId) const;
    };
}