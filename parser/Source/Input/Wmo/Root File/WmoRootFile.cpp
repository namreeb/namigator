#include <vector>
#include <sstream>
#include <iomanip>
#include <map>

#include "Input/WMO/Group File/WmoGroupFile.hpp"
#include "Input/WMO/Root File/Chunks/MOHD.hpp"
#include "Input/WMO/Root File/Chunks/MODS.hpp"
#include "Input/WMO/Root File/Chunks/MODN.hpp"
#include "Input/WMO/Root File/Chunks/MODD.hpp"
#include "Input/M2/DoodadFile.hpp"

#include "MathHelper.hpp"
#include "Misc.hpp"

#include "Input/WMO/Root File/WmoRootFile.hpp"

using namespace utility;

namespace parser_input
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

        const float mid = (float)((533.0 + (1.0/3.0)) * 32.0);
        float xPos = -(info->BasePosition.Z - mid);
        float yPos = -(info->BasePosition.X - mid);
        float zPos = info->BasePosition.Y;
        const Vertex origin(xPos, yPos, zPos);

        xPos = -(info->MaxCorner.Z - mid);
        yPos = -(info->MaxCorner.X - mid);
        zPos = info->MaxCorner.Y;

        const Vertex maxCorner = Vertex(xPos, yPos, zPos);
        
        xPos = -(info->MinCorner.Z - mid);
        yPos = -(info->MinCorner.X - mid);
        zPos = info->MinCorner.Y;

        const Vertex minCorner = Vertex(xPos, yPos, zPos);

        Bounds = BoundingBox(minCorner, maxCorner);

        const float rotX = MathHelper::ToRadians(info->OrientationC);
        const float rotY = MathHelper::ToRadians(info->OrientationA);
        const float rotZ = MathHelper::ToRadians(info->OrientationB + 180.f);    // XXX - other people use -90?
        const Matrix transformMatrix = Matrix::CreateRotationX(rotX) * Matrix::CreateRotationY(rotY) * Matrix::CreateRotationZ(rotZ);

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
                    const Vertex rotatedVertex = origin + Vertex::Transform(groupFiles[g]->VerticesChunk->Vertices[vertexIndex], transformMatrix);

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
            //if (!groupFiles[g]->LiquidChunk)
                continue;

            const float tileSize = (533.f + (1.f / 3.f)) / 128.f;

            // process liquid chunk for the current group file
            for (unsigned int y = 0; y < groupFiles[g]->LiquidChunk->Height; ++y)
                for (unsigned int x = 0; x < groupFiles[g]->LiquidChunk->Width; ++x)
                {
                    const Vertex baseVertex(groupFiles[g]->LiquidChunk->Base[0],
                                            groupFiles[g]->LiquidChunk->Base[1],
                                            groupFiles[g]->LiquidChunk->Base[2]);

                    // XXX - for some reason, this works best when you ignore the height map and use only the base height

                    Vertex v1(x * tileSize, y * tileSize, groupFiles[g]->LiquidChunk->Heights->Get(y, x));
                    v1 = origin + Vertex::Transform(baseVertex + v1, transformMatrix);

                    Vertex v2((x + 1) * tileSize, y * tileSize, groupFiles[g]->LiquidChunk->Heights->Get(y, x + 1));
                    v2 = origin + Vertex::Transform(baseVertex + v2, transformMatrix);

                    Vertex v3(x * tileSize, (y + 1) * tileSize, groupFiles[g]->LiquidChunk->Heights->Get(y + 1, x));
                    v3 = origin + Vertex::Transform(baseVertex + v3, transformMatrix);

                    Vertex v4((x + 1) * tileSize, (y + 1) * tileSize, groupFiles[g]->LiquidChunk->Heights->Get(x + 1, y + 1));
                    v4 = origin + Vertex::Transform(baseVertex + v4, transformMatrix);

                    LiquidVertices.push_back(v1);
                    LiquidVertices.push_back(v2);
                    LiquidVertices.push_back(v3);
                    LiquidVertices.push_back(v4);

                    LiquidIndices.push_back(LiquidVertices.size() - 4);
                    LiquidIndices.push_back(LiquidVertices.size() - 2);
                    LiquidIndices.push_back(LiquidVertices.size() - 3);

                    LiquidIndices.push_back(LiquidVertices.size() - 2);
                    LiquidIndices.push_back(LiquidVertices.size() - 1);
                    LiquidIndices.push_back(LiquidVertices.size() - 3);

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

        std::unique_ptr<MODS> doodadSetsChunk(modsLoc > 0 ? new MODS(information.DoodadSetsCount, modsLoc, Reader) : nullptr);
        std::unique_ptr<MODN> doodadNamesChunk(modnLoc > 0 ? new MODN(information.DoodadNamesCount, modnLoc, Reader) : nullptr);
        std::unique_ptr<MODD> doodadChunk(moddLoc > 0 ? new MODD(moddLoc, Reader) : nullptr);

        std::vector<std::unique_ptr<WmoDoodad>> finalDoodads;
        finalDoodads.resize(doodadChunk->Count);

        for (int i = 0; i < doodadChunk->Count; ++i)
        {
            const unsigned int nameIndex = doodadChunk->Doodads[i].DoodadInfo.NameIndex;
            const unsigned int index = doodadChunk->Doodads[i].Index;

            if (!!doodadSetsChunk && doodadSetsChunk->Count)
            {
                const DoodadSetInfo *set = &doodadSetsChunk->DoodadSets[info->DoodadSet];

                if (index < set->FirstDoodadIndex || index > (set->FirstDoodadIndex + set->DoodadCount - 1))
                    continue;
            }

            finalDoodads[i].reset(new WmoDoodad(info, &doodadChunk->Doodads[i].DoodadInfo, doodadNamesChunk->Names[nameIndex]));
        }

        for (unsigned int i = 0; i < finalDoodads.size(); ++i)
        {
            const WmoDoodad *doodad = finalDoodads[i].get();

            if (!doodad)
                continue;

            if (!doodad->Vertices.size())
                continue;

            const unsigned int oldSize = DoodadVertices.size();
            DoodadVertices.resize(oldSize + doodad->Vertices.size());

            for (unsigned int j = 0; j < doodad->Indices.size(); ++j)
                DoodadIndices.push_back(oldSize + doodad->Indices[j]);

            memcpy((void *)&DoodadVertices[oldSize], &doodad->Vertices[0], sizeof(Vertex) * doodad->Vertices.size());

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