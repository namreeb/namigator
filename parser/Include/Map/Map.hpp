#pragma once

#include <mutex>
#include <memory>
#include <string>
#include <map>
#include <vector>

#ifdef _DEBUG
#include <ostream>
#endif

namespace parser
{
class Wmo;
class WmoInstance;
class Doodad;
class DoodadInstance;
class Adt;

class Map
{
    private:
        bool m_hasAdt[64][64];
        bool m_hasTerrain;

        mutable std::mutex m_adtMutex;
        std::unique_ptr<Adt> m_adts[64][64];
        std::unique_ptr<WmoInstance> m_globalWmo;

        mutable std::mutex m_wmoMutex;
        std::vector<std::unique_ptr<const Wmo>> m_loadedWmos;
        std::map<unsigned int, std::unique_ptr<const WmoInstance>> m_loadedWmoInstances;

        mutable std::mutex m_doodadMutex;
        std::vector<std::unique_ptr<const Doodad>> m_loadedDoodads;
        std::map<unsigned int, std::unique_ptr<const DoodadInstance>> m_loadedDoodadInstances;

    public:
        const std::string Name;

        Map(const std::string &MapName);

        bool HasAdt(int x, int y) const;
        const Adt *GetAdt(int x, int y);
        void UnloadAdt(int x, int y);

        const Wmo *GetWmo(const std::string &name);

        void InsertWmoInstance(unsigned int uniqueId, const WmoInstance *wmo);
        void UnloadWmoInstance(unsigned int uniqueId);
        const WmoInstance *GetWmoInstance(unsigned int uniqueId) const;
        const WmoInstance *GetGlobalWmoInstance() const;

        const Doodad *GetDoodad(const std::string &name);

        void InsertDoodadInstance(unsigned int uniqueId, const DoodadInstance *doodad);
        void UnloadDoodadInstance(unsigned int uniqueId);
        const DoodadInstance *GetDoodadInstance(unsigned int uniqueId) const;

#ifdef _DEBUG
        bool IsAdtLoaded(int x, int y) const;

        void WriteMemoryUsage(std::ostream &stream) const;
#endif
};
}