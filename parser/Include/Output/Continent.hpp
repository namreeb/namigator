#pragma once

#include "parser/Include/Output/Wmo.hpp"
#include "parser/Include/Output/Doodad.hpp"
#include "parser/Include/Output/Adt.hpp"

#include <mutex>
#include <memory>
#include <string>
#include <map>
#include <set>

#ifdef _DEBUG
#include <ostream>
#endif

namespace parser
{
class Continent
{
    private:
        bool m_hasAdt[64][64];
        bool m_hasTerrain;

        mutable std::mutex m_adtMutex;
        std::unique_ptr<Adt> m_adts[64][64];
        std::unique_ptr<Wmo> m_globalWmo;

        mutable std::mutex m_wmoMutex;
        std::map<unsigned int, std::unique_ptr<const Wmo>> m_loadedWmos;

        mutable std::mutex m_doodadMutex;
        std::map<unsigned int, std::unique_ptr<Doodad>> m_loadedDoodads;

    public:
        const std::string Name;

        Continent(const std::string &continentName);

        const Adt *LoadAdt(int x, int y);
        void UnloadAdt(int x, int y);

        bool HasAdt(int x, int y) const;
        bool IsAdtLoaded(int x, int y) const;

        bool IsWmoLoaded(unsigned int uniqueId) const;
        void InsertWmo(unsigned int uniqueId, const Wmo *wmo);
        const Wmo *GetGlobalWmo() const;
        const Wmo *GetWmo(unsigned int uniqueId) const;
        void UnloadWmo(unsigned int uniqueId);

        bool IsDoodadLoaded(unsigned int uniqueId) const;
        void InsertDoodad(unsigned int uniqueId, Doodad *doodad);
        const Doodad *GetDoodad(unsigned int uniqueId) const;
        void UnloadDoodad(unsigned int uniqueId);

#ifdef _DEBUG
		void WriteMemoryUsage(std::ostream &stream) const;
#endif
};
}