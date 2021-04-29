#pragma once

#include "Common.hpp"
#include "parser/Adt/Adt.hpp"
#include "parser/Wmo/WmoInstance.hpp"
#include "utility/BinaryStream.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <ostream>
#include <set>
#include <string>
#include <vector>

namespace parser
{
class Wmo;
class Doodad;
class DoodadInstance;

class Map
{
private:
    bool m_hasAdt[MeshSettings::Adts][MeshSettings::Adts];
    bool m_hasTerrain;

    mutable std::mutex m_adtMutex;
    std::unique_ptr<Adt> m_adts[MeshSettings::Adts][MeshSettings::Adts];
    std::unique_ptr<WmoInstance> m_globalWmo;

    mutable std::mutex m_wmoMutex;
    std::vector<std::unique_ptr<const Wmo>> m_loadedWmos;
    std::map<std::uint32_t, std::unique_ptr<const WmoInstance>>
        m_loadedWmoInstances;
    std::map<std::uint64_t, std::unique_ptr<const WmoInstance>>
        m_loadedWmoGameObjects;

    mutable std::mutex m_doodadMutex;
    std::vector<std::shared_ptr<const Doodad>>
        m_loadedDoodads; // must be shared because it can also be owned by a WMO
    std::map<std::uint32_t, std::unique_ptr<const DoodadInstance>>
        m_loadedDoodadInstances;
    std::map<std::uint64_t, std::unique_ptr<const WmoInstance>>
        m_loadedDoodadGameObjects;

public:
    const std::string Name;
    const unsigned int Id;

    Map(const std::string& MapName);

    bool HasAdt(int x, int y) const;
    const Adt* GetAdt(int x, int y);
    void UnloadAdt(int x, int y);

    const Wmo* GetWmo(const std::string& name);

    void InsertWmoInstance(unsigned int uniqueId, const WmoInstance* wmo);
    const WmoInstance* GetWmoInstance(unsigned int uniqueId) const;
    const WmoInstance* GetGlobalWmoInstance() const;

    const Doodad* GetDoodad(const std::string& name);

    void InsertDoodadInstance(unsigned int uniqueId,
                              const DoodadInstance* doodad);
    const DoodadInstance* GetDoodadInstance(unsigned int uniqueId) const;

    void Serialize(utility::BinaryStream& stream) const;
};
} // namespace parser