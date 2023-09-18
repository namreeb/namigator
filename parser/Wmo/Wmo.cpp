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
#include "utility/String.hpp"

#include <cstdint>
#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <vector>

namespace parser
{
Wmo::Wmo(const std::string& path) : MpqPath(utility::lower(path))
{
    auto reader = sMpqManager.OpenFile(path);

    if (!reader)
        THROW_MSG("WMO " + path + " not found", Result::WMO_NOT_FOUND);

    size_t mverLocation;
    if (!reader->GetChunkLocation("MVER", mverLocation))
        THROW(Result::MVER_NOT_FOUND);

    reader->rpos(mverLocation + 8);
    auto const version = reader->Read<std::uint32_t>();

    size_t mohdLocation;

    // alpha data
    if (version == 14)
    {
        size_t momoLocation;
        if (!reader->GetChunkLocation("MOMO", momoLocation))
            THROW(Result::MOMO_NOT_FOUND);

        if (!reader->GetChunkLocation("MOHD", reader->rpos() + 8, mohdLocation))
            THROW(Result::MOHD_NOT_FOUND);
    }
    else if (!reader->GetChunkLocation("MOHD", mohdLocation))
        THROW(Result::MOHD_NOT_FOUND);

    reader->rpos(mohdLocation + 8);

    input::MOHD information;
    reader->ReadBytes(&information, sizeof(information));

    std::vector<std::unique_ptr<input::WmoGroupFile>> groupFiles;
    groupFiles.reserve(information.WMOGroupFilesCount);

    // alpha data
    if (version == 14)
    {
        for (int i = 0; i < information.WMOGroupFilesCount; ++i)
            groupFiles.push_back(
                std::make_unique<input::WmoGroupFile>(version, reader.get()));
    }
    else
    {
        auto fileName = path.substr(path.rfind('\\') + 1);
        fileName = fileName.substr(0, fileName.rfind('.'));
        auto const dirName = path.substr(0, path.rfind('\\'));

        for (int i = 0; i < information.WMOGroupFilesCount; ++i)
        {
            std::stringstream ss;

            ss << dirName << "\\" << fileName << "_" << std::setfill('0')
               << std::setw(3) << i << ".wmo";
            groupFiles.push_back(
                std::make_unique<input::WmoGroupFile>(version, ss.str()));
        }
    }

    size_t modsLocation;
    if (!reader->GetChunkLocation("MODS", mohdLocation, modsLocation))
        THROW(Result::MODS_NOT_FOUND);

    size_t modnLocation;
    if (!reader->GetChunkLocation("MODN", mohdLocation, modnLocation))
        THROW(Result::MODN_NOT_FOUND);

    size_t moddLocation;
    if (!reader->GetChunkLocation("MODD", mohdLocation, moddLocation))
        THROW(Result::MODD_NOT_FOUND);

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

            // add the vertices/indices for this triangle
            // note: we need to check if this Vector3 has already been added as
            // part of another triangle
            for (int j = 0; j < 3; ++j)
            {
                int vertexIndex;

                // according to https://wowdev.wiki/WMO#MOIN: "It's most of the
                // time only a list incrementing from 0 to nFaces * 3 or less,
                // not always up to nPolygons (calculated with MOPY)."
                // this makes no sense to me at all, but the same article
                // implies that we can use the logic that follows:
                if (version == 14)
                    vertexIndex = i * 3 + j;
                else
                {
                    assert(i * 3 + j <
                           groupFiles[g]->IndicesChunk->Indices.size());
                    vertexIndex =
                        groupFiles[g]->IndicesChunk->Indices[i * 3 + j];

                    // this Vector3 has already been added.  add it's index to
                    // this triangle also
                    if (indexMap.find(vertexIndex) != indexMap.end())
                    {
                        Indices.push_back(indexMap[vertexIndex]);
                        continue;
                    }

                    // add a mapping for the Vector3's old index to its position
                    // in the new Vector3 list (aka its new index)
                    indexMap[vertexIndex] = static_cast<int>(Vertices.size());
                }

                // add the index
                Indices.push_back(static_cast<std::int32_t>(Vertices.size()));

                assert(vertexIndex <
                       groupFiles[g]->VerticesChunk->Vertices.size());

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

        const math::Vector3 baseVertex(liquidChunk->Corner[0],
                                       liquidChunk->Corner[1],
                                       liquidChunk->Corner[2]);

        // process liquid chunk for the current group file
        // XXX - for some reason, this works best when you ignore the height map
        // and use only the base height
        for (unsigned int y = 0; y < liquidChunk->Height; ++y)
            for (unsigned int x = 0; x < liquidChunk->Width; ++x)
            {
                const math::Vector3 v1({baseVertex.X + tileSize * (x + 0),
                                        baseVertex.Y + tileSize * (y + 0),
                                        baseVertex.Z});
                const math::Vector3 v2({baseVertex.X + tileSize * (x + 1),
                                        baseVertex.Y + tileSize * (y + 0),
                                        baseVertex.Z});
                const math::Vector3 v3({baseVertex.X + tileSize * (x + 0),
                                        baseVertex.Y + tileSize * (y + 1),
                                        baseVertex.Z});
                const math::Vector3 v4({baseVertex.X + tileSize * (x + 1),
                                        baseVertex.Y + tileSize * (y + 1),
                                        baseVertex.Z});

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

            auto doodad = LoadDoodad(name);

            if (!!doodad->Vertices.size() && !!doodad->Indices.size())
                DoodadSets[i].push_back(
                    std::make_unique<WmoDoodad const>(doodad, transformMatrix));
        }
    }
}

std::shared_ptr<const Doodad> Wmo::LoadDoodad(const std::string &name) const
{
    for (size_t i = 0; i < DoodadSets.size(); ++i)
    {
        auto & doodadSet = DoodadSets[i];
        for (size_t j = 0; j < doodadSet.size(); ++j)
            if (doodadSet[j]->Parent->MpqPath == name)
                return doodadSet[j]->Parent;
    }

    return std::make_shared<const Doodad>(name);
}
} // namespace parser