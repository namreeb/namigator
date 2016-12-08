#pragma once

#include <mutex>
#include <memory>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <ostream>
#include <cstdint>

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
        std::map<std::uint32_t, std::unique_ptr<const WmoInstance>> m_loadedWmoInstances;
        std::map<std::uint64_t, std::unique_ptr<const WmoInstance>> m_loadedWmoGameObjects;

        mutable std::mutex m_doodadMutex;
        std::vector<std::unique_ptr<const Doodad>> m_loadedDoodads;
        std::map<std::uint32_t, std::unique_ptr<const DoodadInstance>> m_loadedDoodadInstances;
        std::map<std::uint64_t, std::unique_ptr<const WmoInstance>> m_loadedDoodadGameObjects;

    public:
        const std::string Name;
        const unsigned int Id;

        Map(const std::string &MapName);

        bool HasAdt(int x, int y) const;
        const Adt *GetAdt(int x, int y);
        void UnloadAdt(int x, int y);

        const Wmo *GetWmo(const std::string &name);

        void InsertWmoInstance(unsigned int uniqueId, const WmoInstance *wmo);
        const WmoInstance *GetWmoInstance(unsigned int uniqueId) const;
        const WmoInstance *GetGlobalWmoInstance() const;

        const Doodad *GetDoodad(const std::string &name);

        void InsertDoodadInstance(unsigned int uniqueId, const DoodadInstance *doodad);
        const DoodadInstance *GetDoodadInstance(unsigned int uniqueId) const;

        void Serialize(std::ostream& stream) const;
};
}