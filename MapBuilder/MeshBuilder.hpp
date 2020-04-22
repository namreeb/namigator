#pragma once

#include "BVHConstructor.hpp"

#include "parser/Map/Map.hpp"
#include "parser/Wmo/Wmo.hpp"

#include "utility/Vector.hpp"
#include "utility/BinaryStream.hpp"

#include "Common.hpp"

#include <vector>
#include <string>
#include <mutex>
#include <unordered_set>
#include <map>
#include <cstdint>
#include <experimental/filesystem>

namespace meshfiles
{
class File
{
    protected:
        // serialized heightfield and finalized mesh data, mapped by tile id
        std::map<std::pair<std::int32_t, std::int32_t>, utility::BinaryStream> m_tiles;

        mutable std::mutex m_mutex;

        // this function assumes that the mutex has already been locked
        void AddTile(int x, int y, utility::BinaryStream &heightfield, const utility::BinaryStream &mesh);

    public:
        virtual ~File() = default;

        virtual void Serialize(const std::experimental::filesystem::path &filename) const = 0;
};

class ADT : File
{
    private:
        const int m_x;
        const int m_y;

        // serialized data for WMOs and doodad IDs, mapped by tile id within the ADT
        std::map<std::pair<int, int>, utility::BinaryStream> m_wmosAndDoodadIds;

        // serialized data for ADT quad heights, mapped by tile id within the ADT
        std::map<std::pair<int, int>, utility::BinaryStream> m_quadHeights;

    public:
        ADT(int x, int y) : m_x(x), m_y(y) {}

        virtual ~ADT() = default;

        // these x and y arguments refer to the tile x and y
        void AddTile(int x, int y, utility::BinaryStream &wmosAndDoodads, utility::BinaryStream &quadHeights, utility::BinaryStream &heightField, utility::BinaryStream &mesh);

        bool IsComplete() const { return m_tiles.size() == (MeshSettings::TilesPerADT*MeshSettings::TilesPerADT); }
        void Serialize(const std::experimental::filesystem::path &filename) const override;
};

class GlobalWMO : File
{
    public:
        virtual ~GlobalWMO() = default;

        void AddTile(int x, int y, utility::BinaryStream &heightField, utility::BinaryStream &mesh);

        void Serialize(const std::experimental::filesystem::path &filename) const override;
};

void SerializeWmo(const parser::Wmo *wmo, BVHConstructor &constructor);
void SerializeDoodad(const parser::Doodad *doodad, const std::experimental::filesystem::path &path);
}

class MeshBuilder
{
    private:
        std::unique_ptr<parser::Map> m_map;
        BVHConstructor m_bvhConstructor;
        const std::experimental::filesystem::path m_outputPath;

#pragma pack (push, 1)
        struct GameObjectInstance
        {
            std::uint64_t guid;
            std::uint32_t displayId;
            std::uint32_t map;
            float position[3];
            float quaternion[4];
        };
#pragma pack (pop)

        std::vector<GameObjectInstance> m_gameObjectInstances;

        std::map<std::pair<int, int>, std::unique_ptr<meshfiles::ADT>> m_adtsInProgress;
        std::unique_ptr<meshfiles::GlobalWMO> m_globalWMO;

        std::vector<std::pair<int, int>> m_pendingTiles;
        std::vector<int> m_chunkReferences; // this is a fixed size, but it is so big that it can single-handedly overflow the stack

        std::vector<math::Vertex> m_globalWMOVertices;
        std::vector<int> m_globalWMOIndices;

        std::vector<math::Vertex> m_globalWMOLiquidVertices;
        std::vector<int> m_globalWMOLiquidIndices;

        std::vector<math::Vertex> m_globalWMODoodadVertices;
        std::vector<int> m_globalWMODoodadIndices;

        std::unordered_set<std::string> m_bvhWmos;
        std::unordered_set<std::string> m_bvhDoodads;

        float m_minX, m_maxX, m_minY, m_maxY, m_minZ, m_maxZ;

        mutable std::mutex m_mutex;

        size_t m_totalTiles;
        size_t m_completedTiles;

        const int m_logLevel;

        void AddChunkReference(int chunkX, int chunkY);
        void RemoveChunkReference(int chunkX, int chunkY);

        void SerializeWmo(const parser::Wmo *wmo);
        void SerializeDoodad(const parser::Doodad *doodad);

        // these two functions assume ownership of the mutex
        meshfiles::ADT *GetInProgressADT(int x, int y);
        void RemoveADT(const meshfiles::ADT *adt);

    public:
        MeshBuilder(const std::string &outputPath, const std::string &mapName, int logLevel);
        MeshBuilder(const std::string &outputPath, const std::string &mapName, int logLevel, int adtX, int adtY);

        void LoadGameObjects(const std::string &path);

        size_t CompletedTiles() const { return m_completedTiles; }

        bool GetNextTile(int &tileX, int &tileY);
        
        bool IsGlobalWMO() const;

        bool BuildAndSerializeWMOTile(int tileX, int tileY);
        bool BuildAndSerializeMapTile(int tileX, int tileY);

        void SaveMap() const;

        float PercentComplete() const;
};
