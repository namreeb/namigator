#include "Input/WMO/Group File/WmoGroupFile.hpp"
#include "Input/WMO/Root File/Chunks/MOHD.hpp"
#include "Input/WMO/Root File/Chunks/MODS.hpp"
#include "Input/WMO/Root File/Chunks/MODN.hpp"
#include "Input/WMO/Root File/Chunks/MODD.hpp"
#include "Input/M2/DoodadFile.hpp"
#include "Input/WMO/Root File/WmoRootFile.hpp"

#include "utility/Include/MathHelper.hpp"
#include "utility/Include/Misc.hpp"

#include <vector>
#include <sstream>
#include <iomanip>
#include <map>

namespace parser
{
namespace input
{
WmoRootFile::WmoRootFile(const std::string &path, const WmoParserInfo *info) : WowFile(path), FullName(path)
{
    Name = path.substr(path.rfind('\\')+1);
    Name = Name.substr(0, Name.rfind('.'));

    const long mohdPosition = GetChunkLocation("MOHD", 0);

    Reader->SetPosition(mohdPosition + 8);

    MOHD information;
    Reader->ReadBytes(&information, sizeof(information));

    std::vector<std::unique_ptr<WmoGroupFile>> groupFiles;
    groupFiles.reserve(information.WMOGroupFilesCount);

    std::string dirName = path.substr(0, path.rfind('\\'));
    for (int i = 0; i < information.WMOGroupFilesCount; ++i)
    {
        std::stringstream ss;

        ss << dirName << "\\" << Name << "_" << std::setfill('0') << std::setw(3) << i << ".wmo";
        groupFiles.push_back(std::unique_ptr<WmoGroupFile>(new WmoGroupFile(ss.str())));
    }

    // XXX - Check all GetChunkLocation() calls to see if we can replace these 0s with something smarter!

    const long modsLoc = GetChunkLocation("MODS");
    const long modnLoc = GetChunkLocation("MODN");
    const long moddLoc = GetChunkLocation("MODD");

    // PROCESSING...
    // NOTE: The minimum and maximum z values obtained from the data files are not always correct.  Therefore we double check and update them as we build vertices.
    // XXX FIXME we should probably check to make sure that the min/max x and y values can be trusted, too!

    constexpr float mid = 32.f * utility::MathHelper::AdtSize;
    float xPos = -(info->BasePosition.Z - mid);
    float yPos = -(info->BasePosition.X - mid);
    float zPos = info->BasePosition.Y;
    const utility::Vertex origin(xPos, yPos, zPos);

    xPos = -(info->MaxCorner.Z - mid);
    yPos = -(info->MaxCorner.X - mid);
    zPos = info->MaxCorner.Y;

    const utility::Vertex maxCorner(xPos, yPos, zPos);
        
    xPos = -(info->MinCorner.Z - mid);
    yPos = -(info->MinCorner.X - mid);
    zPos = info->MinCorner.Y;

    const utility::Vertex minCorner(xPos, yPos, zPos);

    Bounds = utility::BoundingBox(minCorner, maxCorner);

    const float rotX = utility::Convert::ToRadians(info->OrientationC);
    const float rotY = utility::Convert::ToRadians(info->OrientationA);
    const float rotZ = utility::Convert::ToRadians(info->OrientationB + 180.f);    // XXX - other people use -90?
    const utility::Matrix transformMatrix = utility::Matrix::CreateRotationX(rotX) *
                                            utility::Matrix::CreateRotationY(rotY) *
                                            utility::Matrix::CreateRotationZ(rotZ);

    bool minMaxStarted = false;

    // XXX - analyze wow files to check if the map reduction is needed

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
                const unsigned short vertexIndex = groupFiles[g]->IndicesChunk->Indices[i*3 + j];

                // this vertex has already been added.  add it's index to this triangle also
                if (indexMap.find(vertexIndex) != indexMap.end())
                {
                    Indices.push_back(indexMap[vertexIndex]);
                    continue;
                }

                // this vertex has not yet been added

                // transform into world space
                const utility::Vertex rotatedVertex = origin + utility::Vertex::Transform(groupFiles[g]->VerticesChunk->Vertices[vertexIndex], transformMatrix);

                // add a mapping for the vertex's old index to its position in the new vertex list (aka its new index)
                indexMap[vertexIndex] = Vertices.size();

                // add the index
                Indices.push_back(Vertices.size());

                // add the vertex
                Vertices.push_back(rotatedVertex);

                if (!minMaxStarted)
                {
                    Bounds.MinCorner.Z = Bounds.MaxCorner.Z = rotatedVertex.Z;
                    minMaxStarted = true;
                }
                else
                {
                    if (rotatedVertex.Z < Bounds.MinCorner.Z)
                        Bounds.MinCorner.Z = rotatedVertex.Z;
                    if (rotatedVertex.Z > Bounds.MaxCorner.Z)
                        Bounds.MaxCorner.Z = rotatedVertex.Z;
                }
            }
        }

        // process MLIQ data

        // FIXME XXX wmo liquid is disabled for now until the renderer is working well enough to debug it!
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
                const utility::Vertex v1 = origin + utility::Vertex::Transform({ (x + 0)*tileSize + baseVertex.X, (y + 0)*tileSize + baseVertex.Y, baseVertex.Z }, transformMatrix);
                const utility::Vertex v2 = origin + utility::Vertex::Transform({ (x + 1)*tileSize + baseVertex.X, (y + 0)*tileSize + baseVertex.Y, baseVertex.Z }, transformMatrix);
                const utility::Vertex v3 = origin + utility::Vertex::Transform({ (x + 0)*tileSize + baseVertex.X, (y + 1)*tileSize + baseVertex.Y, baseVertex.Z }, transformMatrix);
                const utility::Vertex v4 = origin + utility::Vertex::Transform({ (x + 1)*tileSize + baseVertex.X, (y + 1)*tileSize + baseVertex.Y, baseVertex.Z }, transformMatrix);

                //auto const h1 = liquidChunk->Heights->Get(y, x);
                //auto const h2 = liquidChunk->Heights->Get(y, x+1);
                //auto const h3 = liquidChunk->Heights->Get(y+1, x);
                //auto const h4 = liquidChunk->Heights->Get(y+1, x+1);

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

                if (!minMaxStarted)
                {
                    Bounds.MinCorner.Z = Bounds.MaxCorner.Z = v1.Z;
                    minMaxStarted = true;
                }

                // check the last four since that is what was just added
                for (unsigned int i = LiquidVertices.size() - 4; i < LiquidVertices.size(); ++i)
                {
                    if (LiquidVertices[i].Z < Bounds.MinCorner.Z)
                        Bounds.MinCorner.Z = LiquidVertices[i].Z;
                    if (LiquidVertices[i].Z > Bounds.MaxCorner.Z)
                        Bounds.MaxCorner.Z = LiquidVertices[i].Z;
                }
            }
    }

    // Doodads...

    MODS doodadSetsChunk(information.DoodadSetsCount, modsLoc, Reader);
    MODN doodadNamesChunk(information.DoodadNamesCount, modnLoc, Reader);
    MODD doodadChunk(moddLoc, Reader);

    std::vector<std::unique_ptr<WmoDoodad>> finalDoodads(doodadChunk.Count);

    // add up vertex count so we can resize the vector only once
    size_t vertexCount = 0;

    for (size_t i = 0; i < finalDoodads.size(); ++i)
    {
        const unsigned int nameIndex = doodadChunk.Doodads[i].DoodadInfo.NameIndex;
        const unsigned int index = doodadChunk.Doodads[i].Index;

        if (doodadSetsChunk.Count)
        {
            const DoodadSetInfo *set = &doodadSetsChunk.DoodadSets[info->DoodadSet];

            if (index < set->FirstDoodadIndex || index > (set->FirstDoodadIndex + set->DoodadCount - 1))
                continue;
        }

        finalDoodads[i].reset(new WmoDoodad(info, &doodadChunk.Doodads[i].DoodadInfo, doodadNamesChunk.Names[nameIndex]));
        vertexCount += finalDoodads[i]->Vertices.size();
    }

    DoodadVertices.resize(vertexCount);

    // re-use this variable to track the re-indexing of doodad triangles
    vertexCount = 0;

    for (size_t i = 0; i < finalDoodads.size(); ++i)
    {
        const WmoDoodad *doodad = finalDoodads[i].get();

        if (!doodad || !doodad->Vertices.size() || !doodad->Indices.size())
            continue;

        for (size_t j = 0; j < doodad->Indices.size(); ++j)
            DoodadIndices.push_back(vertexCount + doodad->Indices[j]);

        memcpy((void *)&DoodadVertices[vertexCount], &doodad->Vertices[0], sizeof(utility::Vertex) * doodad->Vertices.size());
        vertexCount += doodad->Vertices.size();

        if (!minMaxStarted)
        {
            Bounds.MinCorner.Z = doodad->MinZ;
            Bounds.MaxCorner.Z = doodad->MaxZ;
            minMaxStarted = true;
        }
        else
        {
            if (doodad->MinZ < Bounds.MinCorner.Z)
                Bounds.MinCorner.Z = doodad->MinZ;
            if (doodad->MaxZ > Bounds.MaxCorner.Z)
                Bounds.MaxCorner.Z = doodad->MaxZ;
        }
    }
}
}
}