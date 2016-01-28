#include "MpqManager.hpp"
#include "Map/Map.hpp"
#include "Wmo/Wmo.hpp"
#include "Wmo/Group File/WmoGroupFile.hpp"
#include "Wmo/Root File/Chunks/MOHD.hpp"
#include "Wmo/Root File/Chunks/MODS.hpp"
#include "Wmo/Root File/Chunks/MODN.hpp"
#include "Wmo/Root File/Chunks/MODD.hpp"
#include "Wmo/WmoDoodadPlacement.hpp"

#include "utility/Include/BinaryStream.hpp"
#include "utility/Include/MathHelper.hpp"
#include "utility/Include/Exception.hpp"

#include <vector>
#include <sstream>
#include <iomanip>
#include <map>
#include <memory>

namespace parser
{
Wmo::Wmo(Map *map, const std::string &path) : FullPath(path)
{
    FileName = path.substr(path.rfind('\\') + 1);
    FileName = FileName.substr(0, FileName.rfind('.'));

    std::unique_ptr<utility::BinaryStream> reader(MpqManager::OpenFile(path));

    size_t mohdLocation;
    if (!reader->GetChunkLocation("MOHD", mohdLocation))
        THROW("MOHD not found");

    reader->SetPosition(mohdLocation + 8);

    input::MOHD information;
    reader->ReadBytes(&information, sizeof(information));

    std::vector<std::unique_ptr<input::WmoGroupFile>> groupFiles;
    groupFiles.reserve(information.WMOGroupFilesCount);

    std::string dirName = path.substr(0, path.rfind('\\'));
    for (int i = 0; i < information.WMOGroupFilesCount; ++i)
    {
        std::stringstream ss;

        ss << dirName << "\\" << FileName << "_" << std::setfill('0') << std::setw(3) << i << ".wmo";
        groupFiles.push_back(std::unique_ptr<input::WmoGroupFile>(new input::WmoGroupFile(ss.str())));
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

    // for each group file
    for (int g = 0; g < information.WMOGroupFilesCount; ++g)
    {
        Vertices.reserve(Vertices.capacity() + groupFiles[g]->VerticesChunk->Vertices.size());
        Indices.reserve(Indices.capacity() + groupFiles[g]->IndicesChunk->Indices.size());

        // process MOPY data

        // maps a vertex index from the .wmo to it's index in our new index/vertex list.
        // this is necessary because the indices will change as non-collideable vertices are discarded.
        // it also has the side-effect of ensuring we have no duplicate vertices

        std::map<unsigned short, int> indexMap;

        // process each triangle in the current group file
        for (int i = 0; i < groupFiles[g]->MaterialsChunk->TriangleCount; ++i)
        {
            // flags != 0x04 implies collision.  materialId = 0xFF (-1) implies a non-rendered collideable triangle.
            if (groupFiles[g]->MaterialsChunk->Flags[i] & 0x04 && groupFiles[g]->MaterialsChunk->MaterialId[i] != 0xFF)
                continue;

            // add the vertices/indices for this triangle
            // note: we need to check if this vertex has already been added as part of another triangle
            for (int j = 0; j < 3; ++j)
            {
                const unsigned short vertexIndex = groupFiles[g]->IndicesChunk->Indices[i * 3 + j];

                // this vertex has already been added.  add it's index to this triangle also
                if (indexMap.find(vertexIndex) != indexMap.end())
                {
                    Indices.push_back(indexMap[vertexIndex]);
                    continue;
                }

                // add a mapping for the vertex's old index to its position in the new vertex list (aka its new index)
                indexMap[vertexIndex] = Vertices.size();

                // add the index
                Indices.push_back(Vertices.size());

                // add the vertex
                Vertices.push_back(groupFiles[g]->VerticesChunk->Vertices[vertexIndex]);
            }
        }

        // process MLIQ data

        if (!groupFiles[g]->LiquidChunk)
            continue;

        constexpr float tileSize = utility::MathHelper::AdtSize / 128.f;
        auto const liquidChunk = groupFiles[g]->LiquidChunk.get();

        const utility::Vertex baseVertex(liquidChunk->Base[0], liquidChunk->Base[1], liquidChunk->Base[2]);

        // process liquid chunk for the current group file
        // XXX - for some reason, this works best when you ignore the height map and use only the base height
        for (unsigned int y = 0; y < liquidChunk->Height; ++y)
            for (unsigned int x = 0; x < liquidChunk->Width; ++x)
            {
                const utility::Vertex v1({ (x + 0)*tileSize + baseVertex.X, (y + 0)*tileSize + baseVertex.Y, baseVertex.Z });
                const utility::Vertex v2({ (x + 1)*tileSize + baseVertex.X, (y + 0)*tileSize + baseVertex.Y, baseVertex.Z });
                const utility::Vertex v3({ (x + 0)*tileSize + baseVertex.X, (y + 1)*tileSize + baseVertex.Y, baseVertex.Z });
                const utility::Vertex v4({ (x + 1)*tileSize + baseVertex.X, (y + 1)*tileSize + baseVertex.Y, baseVertex.Z });

                LiquidVertices.push_back(v1);
                LiquidVertices.push_back(v2);
                LiquidVertices.push_back(v3);
                LiquidVertices.push_back(v4);

                LiquidIndices.push_back(LiquidVertices.size() - 4);
                LiquidIndices.push_back(LiquidVertices.size() - 3);
                LiquidIndices.push_back(LiquidVertices.size() - 2);

                LiquidIndices.push_back(LiquidVertices.size() - 2);
                LiquidIndices.push_back(LiquidVertices.size() - 3);
                LiquidIndices.push_back(LiquidVertices.size() - 1);
            }
    }

    // Doodad sets..

    input::MODS doodadSetsChunk(information.DoodadSetsCount, modsLocation, reader.get());
    input::MODN doodadNamesChunk(information.DoodadNamesCount, modnLocation, reader.get());
    input::MODD doodadChunk(moddLocation, reader.get());

    DoodadSets.resize(doodadSetsChunk.DoodadSets.size());

    for (size_t i = 0; i < doodadSetsChunk.DoodadSets.size(); ++i)
    {
        auto const &set = doodadSetsChunk.DoodadSets[i];

        DoodadSets[i].reserve(set.DoodadCount);

        for (auto d = set.FirstDoodadIndex; d < set.FirstDoodadIndex + set.DoodadCount; ++d)
        {
            auto const &placement = doodadChunk.Doodads[d];
            auto const &name = doodadNamesChunk.Names[placement.NameIndex];

            utility::Matrix transformMatrix;
            placement.GetTransformMatrix(transformMatrix);

            DoodadSets[i].push_back(std::unique_ptr<const WmoDoodad>(new WmoDoodad(map->GetDoodad(name), placement.Position, transformMatrix)));
        }
    }
}

#ifdef _DEBUG
unsigned int Wmo::MemoryUsage() const
{
    unsigned int ret = sizeof(Wmo);

    ret += FullPath.length();
    ret += FileName.length();

    ret += Vertices.size() * sizeof(Vertices[0]);
    ret += Indices.size() * sizeof(Indices[0]);
    ret += LiquidVertices.size() * sizeof(LiquidVertices[0]);
    ret += LiquidIndices.size() * sizeof(LiquidIndices[0]);
    
    ret += DoodadSets.size() * sizeof(DoodadSets[0]);

    return ret;
}
#endif
}