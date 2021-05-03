#include "Wmo/Wmo.hpp"

#include "Common.hpp"
#include "DBC.hpp"
#include "MpqManager.hpp"
#include "Wmo/GroupFile/WmoGroupFile.hpp"
#include "Wmo/RootFile/Chunks/MODD.hpp"
#include "Wmo/RootFile/Chunks/MODN.hpp"
#include "Wmo/RootFile/Chunks/MODS.hpp"
#include "Wmo/RootFile/Chunks/MOHD.hpp"
#include "Wmo/WmoDoodadPlacement.hpp"
#include "utility/BinaryStream.hpp"
#include "utility/Exception.hpp"

#include <cstdint>
#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <vector>

namespace parser
{
Wmo::Wmo(const std::string& path) : MpqPath(path)
{
    auto fileName = path.substr(path.rfind('\\') + 1);
    fileName = fileName.substr(0, fileName.rfind('.'));

    auto reader = sMpqManager.OpenFile(path);

    if (!reader)
        THROW("WMO " + path + " not found");

    size_t mohdLocation;
    if (!reader->GetChunkLocation("MOHD", mohdLocation))
        THROW("MOHD not found");

    reader->rpos(mohdLocation + 8);

    input::MOHD information;
    reader->ReadBytes(&information, sizeof(information));

    std::vector<std::unique_ptr<input::WmoGroupFile>> groupFiles;
    groupFiles.reserve(information.WMOGroupFilesCount);

    std::string dirName = path.substr(0, path.rfind('\\'));
    for (int i = 0; i < information.WMOGroupFilesCount; ++i)
    {
        std::stringstream ss;

        ss << dirName << "\\" << fileName << "_" << std::setfill('0')
           << std::setw(3) << i << ".wmo";
        groupFiles.push_back(std::make_unique<input::WmoGroupFile>(ss.str()));
    }

    size_t modsLocation;
    if (!reader->GetChunkLocation("MODS", mohdLocation, modsLocation))
        THROW("MODS not found");

    size_t modnLocation;
    if (!reader->GetChunkLocation("MODN", mohdLocation, modnLocation))
        THROW("MODN not found");

    size_t moddLocation;
    if (!reader->GetChunkLocation("MODD", mohdLocation, moddLocation))
        THROW("MODD not found");

    // PROCESSING...

    // TODO: this is PROBABLY wrong, but it seems like most (all?) WMOs use a
    // single AreaTable ID for the entire WMO.  But which ID is used can vary
    // depending on which "name set" is selected for a particular instance of
    // the WMO.  Therefore let us save a list of tuples in the form:
    // (name set, area id)
    // This ASSUMES that there will only be one area ID per WMO.  This is
    // probably true, but the data format allows for the violation of the
    // assumption.
    const DBC wmoAreaTable("DBFilesClient\\WMOAreaTable.dbc");
    std::unordered_map<unsigned int, unsigned int> nameSetToAreaId;
    for (auto i = 0; i < wmoAreaTable.RecordCount(); ++i)
    {
        auto const rootId = wmoAreaTable.GetField(i, 1);

        // not for this wmo? skip
        if (rootId != information.WMOId)
            continue;

        auto const areaId = wmoAreaTable.GetField(i, 10);

        if (areaId == 0)
            continue;

        auto const nameSet = wmoAreaTable.GetField(i, 2);

        auto const it = nameSetToAreaId.find(nameSet);
        if (it != nameSetToAreaId.end() && it->second != areaId)
        {
            int asdf = 1234;
        }

        nameSetToAreaId[nameSet] = areaId;
    }

    std::vector<std::array<unsigned int, 3>> f;

    NameSetToAreaAndZone.reserve(nameSetToAreaId.size());
    for (auto const& it : nameSetToAreaId)
        NameSetToAreaAndZone.push_back(
            {it.first, it.second, sMpqManager.GetZoneId(it.second)});

    RootId = information.WMOId;

    // for each group file
    for (int g = 0; g < information.WMOGroupFilesCount; ++g)
    {
        Vertices.reserve(Vertices.capacity() +
                         groupFiles[g]->VerticesChunk->Vertices.size());
        Indices.reserve(Indices.capacity() +
                        groupFiles[g]->IndicesChunk->Indices.size());

        // process MOPY data

        // maps a vertex index from the .wmo to it's index in our new
        // index/vertex list. this is necessary because the indices will change
        // as non-collideable vertices are discarded. it also has the
        // side-effect of ensuring we have no duplicate vertices

        std::map<std::int16_t, int> indexMap;

        // process each triangle in the current group file
        for (int i = 0; i < groupFiles[g]->MaterialsChunk->TriangleCount; ++i)
        {
            // flags != 0x04 implies collision.  materialId = 0xFF (-1) implies
            // a non-rendered collideable triangle.
            if (groupFiles[g]->MaterialsChunk->Flags[i] & 0x04 &&
                groupFiles[g]->MaterialsChunk->MaterialId[i] != 0xFF)
                continue;

            // 0x40 is unwalkable
            if (groupFiles[g]->MaterialsChunk->Flags[i] & 0x40)
                continue;

            // add the vertices/indices for this triangle
            // note: we need to check if this Vector3 has already been added as
            // part of another triangle
            for (int j = 0; j < 3; ++j)
            {
                auto const vertexIndex =
                    groupFiles[g]->IndicesChunk->Indices[i * 3 + j];

                // this Vector3 has already been added.  add it's index to this
                // triangle also
                if (indexMap.find(vertexIndex) != indexMap.end())
                {
                    Indices.push_back(indexMap[vertexIndex]);
                    continue;
                }

                // add a mapping for the Vector3's old index to its position in
                // the new Vector3 list (aka its new index)
                indexMap[vertexIndex] = static_cast<int>(Vertices.size());

                // add the index
                Indices.push_back(static_cast<std::int32_t>(Vertices.size()));

                // add the Vector3
                Vertices.push_back(
                    groupFiles[g]->VerticesChunk->Vertices[vertexIndex]);
            }
        }

        // process MLIQ data

        if (!groupFiles[g]->LiquidChunk)
            continue;

        constexpr float tileSize = MeshSettings::AdtSize / 128.f;
        auto const liquidChunk = groupFiles[g]->LiquidChunk.get();

        const math::Vector3 baseVector3(
            liquidChunk->Base[0], liquidChunk->Base[1], liquidChunk->Base[2]);

        // process liquid chunk for the current group file
        // XXX - for some reason, this works best when you ignore the height map
        // and use only the base height
        for (unsigned int y = 0; y < liquidChunk->Height; ++y)
            for (unsigned int x = 0; x < liquidChunk->Width; ++x)
            {
                const math::Vector3 v1({(x + 0) * tileSize + baseVector3.X,
                                        (y + 0) * tileSize + baseVector3.Y,
                                        baseVector3.Z});
                const math::Vector3 v2({(x + 1) * tileSize + baseVector3.X,
                                        (y + 0) * tileSize + baseVector3.Y,
                                        baseVector3.Z});
                const math::Vector3 v3({(x + 0) * tileSize + baseVector3.X,
                                        (y + 1) * tileSize + baseVector3.Y,
                                        baseVector3.Z});
                const math::Vector3 v4({(x + 1) * tileSize + baseVector3.X,
                                        (y + 1) * tileSize + baseVector3.Y,
                                        baseVector3.Z});

                LiquidVertices.push_back(v1);
                LiquidVertices.push_back(v2);
                LiquidVertices.push_back(v3);
                LiquidVertices.push_back(v4);

                LiquidIndices.push_back(
                    static_cast<std::int32_t>(LiquidVertices.size() - 4));
                LiquidIndices.push_back(
                    static_cast<std::int32_t>(LiquidVertices.size() - 3));
                LiquidIndices.push_back(
                    static_cast<std::int32_t>(LiquidVertices.size() - 2));

                LiquidIndices.push_back(
                    static_cast<std::int32_t>(LiquidVertices.size() - 2));
                LiquidIndices.push_back(
                    static_cast<std::int32_t>(LiquidVertices.size() - 3));
                LiquidIndices.push_back(
                    static_cast<std::int32_t>(LiquidVertices.size() - 1));
            }
    }

    // Doodad sets

    input::MODS doodadSetsChunk(information.DoodadSetsCount, modsLocation,
                                reader.get());
    input::MODN doodadNamesChunk(information.DoodadNamesCount, modnLocation,
                                 reader.get());
    input::MODD doodadChunk(moddLocation, reader.get());

    DoodadSets.resize(doodadSetsChunk.DoodadSets.size());

    for (size_t i = 0; i < doodadSetsChunk.DoodadSets.size(); ++i)
    {
        auto const& set = doodadSetsChunk.DoodadSets[i];

        DoodadSets[i].reserve(set.DoodadCount);

        for (auto d = set.FirstDoodadIndex;
             d < set.FirstDoodadIndex + set.DoodadCount; ++d)
        {
            auto const& placement = doodadChunk.Doodads[d];
            auto const& name = doodadNamesChunk.Names[placement.NameIndex];

            // TODO: figure out why this happens
            if (name.empty())
                continue;

            math::Matrix transformMatrix;
            placement.GetTransformMatrix(transformMatrix);

            auto doodad = std::make_shared<const Doodad>(name);

            if (!!doodad->Vertices.size() && !!doodad->Indices.size())
                DoodadSets[i].push_back(
                    std::make_unique<WmoDoodad const>(doodad, transformMatrix));
        }
    }
}
} // namespace parser