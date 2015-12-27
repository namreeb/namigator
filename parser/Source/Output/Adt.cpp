#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <set>
#include <memory>

#include "Input/Adt/AdtFile.hpp"
#include "Output/Adt.hpp"
#include "Output/Continent.hpp"
#include "LinearAlgebra.hpp"
#include "parser.hpp"
#include "Directory.hpp"

namespace parser
{
    inline bool Adt::IsRendered(unsigned char mask[], int x, int y)
    {
        return (mask[y] >> x & 1) != 0;
    }

    Adt::Adt(Continent *continent, int x, int y)
        : X(x), Y(y),
          MaxX((16.f-(float)x)*(533.f+(1.f/3.f))), MinX(MaxX - (533.f + (1.f/3.f))),
          MaxY((16.f-(float)y)*(533.f+(1.f/3.f))), MinY(MaxY - (533.f + (1.f/3.f))), m_continent(continent)
    {
        std::stringstream ss;

        ss << "World\\Maps\\" << continent->Name << "\\" << continent->Name << "_" << x << "_" << y << ".adt";

        // parsing
        parser_input::AdtFile adt(ss.str());

#ifdef DEBUG
        if (adt.GetChunkLocation("MCLQ", 0) >= 0)
            THROW("ADT base has MCLQ thought to be removed");
#endif

        // Process all data into triangles/indices
        // Terrain

        for (int chunkY = 0; chunkY < 16; ++chunkY)
        {
            for (int chunkX = 0; chunkX < 16; ++chunkX)
            {
                const parser_input::MCNK *mapChunk = adt.m_chunks[chunkY][chunkX].get();
                AdtChunk *const chunk = new AdtChunk();

                int vertCount = 0;

                chunk->m_areaId = mapChunk->AreaId;
                chunk->m_terrainVertices = std::move(mapChunk->Positions);

                for (int i = 0; i < 145; ++i)
                    chunk->m_terrainHeights[i] = mapChunk->HeightChunk->Heights[i] + mapChunk->Height;

                memcpy(chunk->m_holeMap, mapChunk->HoleMap, sizeof(bool)*8*8);

                // build index list to exclude holes (8 * 8 quads, 4 triangles per quad, 3 indices per triangle)
                chunk->m_terrainIndices.reserve(8 * 8 * 4 * 3);

                for (int y = 0; y < 8; ++y)
                {
                    for (int x = 0; x < 8; ++x)
                    {
                        const int currIndex = y * 17 + x;

                        // if this chunk has holes and this quad is one of them, skip it
                        if (mapChunk->HasHoles && mapChunk->HoleMap[y][x])
                            continue;

                        // Upper triangle
                        chunk->m_terrainIndices.push_back(currIndex);
                        chunk->m_terrainIndices.push_back(currIndex + 9);
                        chunk->m_terrainIndices.push_back(currIndex + 1);

                        // Left triangle
                        chunk->m_terrainIndices.push_back(currIndex);
                        chunk->m_terrainIndices.push_back(currIndex + 17);
                        chunk->m_terrainIndices.push_back(currIndex + 9);

                        // Lower triangle
                        chunk->m_terrainIndices.push_back(currIndex + 9);
                        chunk->m_terrainIndices.push_back(currIndex + 17);
                        chunk->m_terrainIndices.push_back(currIndex + 18);

                        // Right triangle
                        chunk->m_terrainIndices.push_back(currIndex + 1);
                        chunk->m_terrainIndices.push_back(currIndex + 9);
                        chunk->m_terrainIndices.push_back(currIndex + 18);
                    }
                }

                m_chunks[chunkY][chunkX].reset(chunk);
            }
        }

        // Water

        if (adt.m_hasMH2O)
        {
            for (int chunkY = 0; chunkY < 16; ++chunkY)
                for (int chunkX = 0; chunkX < 16; ++chunkX)
                {
                    auto chunk = m_chunks[chunkY][chunkX].get();
                    //MCNK *mapChunk = adt.m_chunks[chunkY][chunkX].get();
                    parser_input::MH2OBlock *liquidBlock = adt.m_liquidChunk->Blocks[chunkY][chunkX];

                    if (liquidBlock)
                    {
                        // expand LiquidVertices.  4 verts per square.
                        chunk->m_liquidVertices.reserve(chunk->m_liquidVertices.size() + 4 * liquidBlock->Data.Width * liquidBlock->Data.Height);

                        // expand LiquidIndices.  6 indices per square (two triangles, three indices per triangle)
                        chunk->m_liquidIndices.reserve(chunk->m_liquidIndices.size() + 6 * liquidBlock->Data.Width * liquidBlock->Data.Height);

                        for (int y = liquidBlock->Data.YOffset; y < liquidBlock->Data.YOffset + liquidBlock->Data.Height; ++y)
                             for (int x = liquidBlock->Data.XOffset; x < liquidBlock->Data.XOffset + liquidBlock->Data.Width; ++x)
                             {
                                 if (!IsRendered(liquidBlock->RenderMask, x, y))
                                     continue;

                                 int terrainVert = y * 17 + x;

                                 chunk->m_liquidVertices.push_back(Vertex(chunk->m_terrainVertices[terrainVert].X,
                                                                          chunk->m_terrainVertices[terrainVert].Y,
                                                                          liquidBlock->Heights[y][x]));

                                 terrainVert = y * 17 + (x + 1);

                                 chunk->m_liquidVertices.push_back(Vertex(chunk->m_terrainVertices[terrainVert].X,
                                                                          chunk->m_terrainVertices[terrainVert].Y,
                                                                          liquidBlock->Heights[y][x]));

                                 terrainVert = (y + 1) * 17 + x;

                                 chunk->m_liquidVertices.push_back(Vertex(chunk->m_terrainVertices[terrainVert].X,
                                                                          chunk->m_terrainVertices[terrainVert].Y,
                                                                          liquidBlock->Heights[y][x]));

                                 terrainVert = (y + 1) * 17 + (x + 1);

                                 chunk->m_liquidVertices.push_back(Vertex(chunk->m_terrainVertices[terrainVert].X,
                                                                          chunk->m_terrainVertices[terrainVert].Y,
                                                                          liquidBlock->Heights[y][x]));

                                 chunk->m_liquidIndices.push_back(chunk->m_liquidIndices.size() - 4);
                                 chunk->m_liquidIndices.push_back(chunk->m_liquidIndices.size() - 2);
                                 chunk->m_liquidIndices.push_back(chunk->m_liquidIndices.size() - 3);

                                 chunk->m_liquidIndices.push_back(chunk->m_liquidIndices.size() - 2);
                                 chunk->m_liquidIndices.push_back(chunk->m_liquidIndices.size() - 1);
                                 chunk->m_liquidIndices.push_back(chunk->m_liquidIndices.size() - 3);
                             }
                    }
                }
        }

        // WMOs

        if (adt.m_wmoChunk)
        {
            std::vector<std::unique_ptr<parser_input::WmoRootFile>> wmoFiles;
            wmoFiles.reserve(adt.m_wmoChunk->Wmos.size());

            for (unsigned int wmoFile = 0; wmoFile < adt.m_wmoChunk->Wmos.size(); ++wmoFile)
            {
                std::string wmoName = adt.m_wmoNames[adt.m_wmoChunk->Wmos[wmoFile].NameId];
                wmoFiles.push_back(std::unique_ptr<parser_input::WmoRootFile>(new parser_input::WmoRootFile(wmoName, &adt.m_wmoChunk->Wmos[wmoFile])));
            }

            for (int chunkY = 0; chunkY < 16; ++chunkY)
                for (int chunkX = 0; chunkX < 16; ++chunkX)
                {
                    //MCNK *mapChunk = adt.m_wmoChunk[chunkY][chunkX];
                    AdtChunk *chunk = m_chunks[chunkY][chunkX].get();

                    BoundingBox chunkBox(chunk->m_terrainVertices[144].X, chunk->m_terrainVertices[144].Y,
                        chunk->m_terrainVertices[0].X, chunk->m_terrainVertices[0].Y);

                    // iterate over each wmo referenced in the adt.  see if it belongs in this chunk and has not been placed yet
                    for (unsigned int w = 0; w < adt.m_wmoChunk->Wmos.size(); ++w)
                    {
                        const unsigned int uniqueId = adt.m_wmoChunk->Wmos[w].UniqueId;
                        bool landsOnChunk = false;

                        // save time by checking overlap with the boundary of the wmo
                        if (!chunkBox.Intersects(wmoFiles[w]->Bounds))
                        {
#ifdef DEBUG
                            if (landsOnChunk)
                                THROW("ADT object file says WMO lands on this chunk, but bounding box disagrees");
#endif
                            continue;
                        }

                        // XXX - this can be made faster by flagging which chunks are hit as the vertices are transformed
                        for (unsigned int i = 0; i < wmoFiles[w]->Vertices.size(); ++i)
                        {
                            if (chunkBox.Contains(wmoFiles[w]->Vertices[i]))
                            {
                                landsOnChunk = true;
                                break;
                            }
                        }

                        if (!landsOnChunk)
                            continue;

                        // this wmo lands on the current chunk.  add it to the chunk's list of unique wmo ids.
                        chunk->m_wmos.insert(uniqueId);

                        // if this is the first time loading this wmo instance for this continent, update the continent
                        if (continent->HasWmo(uniqueId))
                            continue;

                        continent->InsertWmo(uniqueId, new Wmo(wmoFiles[w]->Vertices, wmoFiles[w]->Indices,
                            wmoFiles[w]->LiquidVertices, wmoFiles[w]->LiquidIndices,
                            wmoFiles[w]->DoodadVertices, wmoFiles[w]->DoodadIndices,
                            wmoFiles[w]->Bounds.MinCorner.Z, wmoFiles[w]->Bounds.MaxCorner.Z));
                    }
                }
        }

        // Doodads

        if (adt.m_doodadChunk)
        {
            std::vector<std::unique_ptr<parser_input::Doodad>> doodads;
            doodads.reserve(adt.m_doodadChunk->Doodads.size());

            for (unsigned int doodadFile = 0; doodadFile < adt.m_doodadChunk->Doodads.size(); ++doodadFile)
            {
                std::string modelName = adt.m_doodadNames[adt.m_doodadChunk->Doodads[doodadFile].NameId];
                doodads.push_back(std::unique_ptr<parser_input::Doodad>(new parser_input::Doodad(modelName, adt.m_doodadChunk->Doodads[doodadFile])));
            }

            for (int chunkY = 0; chunkY < 16; ++chunkY)
                for (int chunkX = 0; chunkX < 16; ++chunkX)
                {
                    //MCNK *mapChunk = adt.m_chunks[chunkY][chunkX].get();
                    auto chunk = m_chunks[chunkY][chunkX].get();

                    const Vector2 upperLeftCorner(chunk->m_terrainVertices[0].X, chunk->m_terrainVertices[0].Y),
                                  lowerRightCorner(chunk->m_terrainVertices[144].X, chunk->m_terrainVertices[144].Y);

                    for (unsigned int d = 0; d < adt.m_doodadChunk->Doodads.size(); ++d)
                    {
                        const unsigned int uniqueId = adt.m_doodadChunk->Doodads[d].UniqueId;

                        bool landsOnChunk = false;

                        for (unsigned int i = 0; i < doodads[d]->Vertices.size(); ++i)
                        {
                            if (doodads[d]->Vertices[i].X > upperLeftCorner.X ||
                                doodads[d]->Vertices[i].X < lowerRightCorner.X ||
                                doodads[d]->Vertices[i].Y > upperLeftCorner.Y ||
                                doodads[d]->Vertices[i].Y < lowerRightCorner.Y)
                                continue;

                            landsOnChunk = true;
                            break;
                        }

                        if (!landsOnChunk)
                            continue;

                        chunk->m_doodads.insert(uniqueId);

                        if (continent->HasDoodad(uniqueId))
                            continue;

                        continent->InsertDoodad(uniqueId, new Doodad(doodads[d]->Vertices, doodads[d]->Indices, doodads[d]->MinZ, doodads[d]->MaxZ));
                    }
                }
        }
    }

    void Adt::WriteObjFile() const
    {
        std::stringstream ss;

        ss << m_continent->Name << "_" << X  << "_" << Y << ".obj";

        std::ofstream out(ss.str());

        unsigned int indexOffset = 1;
        std::set<unsigned int> dumpedWmos;
        std::set<unsigned int> dumpedDoodads;

        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 16; ++x)
            {
                auto chunk = m_chunks[y][x].get();

                // terrain vertices
                out << "# terrain vertices (" << chunk->m_terrainVertices.size() << ")" << std::endl;
                for (unsigned int i = 0; i < chunk->m_terrainVertices.size(); ++i)
                    out << "v " << -chunk->m_terrainVertices[i].Y
                        << " "  <<  chunk->m_terrainVertices[i].Z
                        << " "  << -chunk->m_terrainVertices[i].X << std::endl;

                // terrain indices
                out << "# terrain indices (" << chunk->m_terrainIndices.size() << ")" << std::endl;
                for (unsigned int i = 0; i < chunk->m_terrainIndices.size(); i += 3)
                    out << "f " << indexOffset + chunk->m_terrainIndices[i]
                        << " "  << indexOffset + chunk->m_terrainIndices[i+1]
                        << " "  << indexOffset + chunk->m_terrainIndices[i+2] << std::endl;

                indexOffset += chunk->m_terrainVertices.size();

                // water vertices
                out << "# water vertices (" << chunk->m_liquidVertices.size() << ")" << std::endl;
                for (unsigned int i = 0; i < chunk->m_liquidVertices.size(); ++i)
                    out << "v " << -chunk->m_liquidVertices[i].Y
                        << " "  <<  chunk->m_liquidVertices[i].Z
                        << " "  << -chunk->m_liquidVertices[i].X << std::endl;

                // water indices
                out << "# water indices (" << chunk->m_liquidIndices.size() << ")" << std::endl;
                for (unsigned int i = 0; i < chunk->m_liquidIndices.size(); i += 3)
                    out << "f " << indexOffset + chunk->m_liquidIndices[i]   << " "
                                << indexOffset + chunk->m_liquidIndices[i+1] << " "
                                << indexOffset + chunk->m_liquidIndices[i+2] << std::endl;

                indexOffset += chunk->m_liquidVertices.size();

                // wmo vertices and indices
                out << "# wmo vertices / indices (including liquids and doodads)" << std::endl;
                for (auto id = chunk->m_wmos.begin(); id != chunk->m_wmos.end(); ++id)
                {
                    // if this wmo has already been dumped as part of another chunk, skip it
                    if (dumpedWmos.find(*id) != dumpedWmos.end())
                        continue;

                    dumpedWmos.insert(*id);

                    Wmo *wmo = m_continent->GetWmo(*id);

                    for (unsigned int i = 0; i < wmo->Vertices.size(); ++i)
                        out << "v " << -wmo->Vertices[i].Y
                            << " "  <<  wmo->Vertices[i].Z
                            << " "  << -wmo->Vertices[i].X << std::endl;

                    for (unsigned int i = 0; i < wmo->Indices.size(); i+=3)
                        out << "f " << indexOffset + wmo->Indices[i]   << " " 
                                    << indexOffset + wmo->Indices[i+1] << " "
                                    << indexOffset + wmo->Indices[i+2] << std::endl;

                    indexOffset += wmo->Vertices.size();

                    for (unsigned int i = 0; i < wmo->LiquidVertices.size(); ++i)
                        out << "v " << -wmo->LiquidVertices[i].Y
                            << " "  <<  wmo->LiquidVertices[i].Z
                            << " "  << -wmo->LiquidVertices[i].X << std::endl;

                    for (unsigned int i = 0; i < wmo->LiquidIndices.size(); i+=3)
                        out << "f " << indexOffset + wmo->LiquidIndices[i]   << " " 
                                    << indexOffset + wmo->LiquidIndices[i+1] << " "
                                    << indexOffset + wmo->LiquidIndices[i+2] << std::endl;

                    indexOffset += wmo->LiquidVertices.size();

                    for (unsigned int i = 0; i < wmo->DoodadVertices.size(); ++i)
                        out << "v " << -wmo->DoodadVertices[i].Y
                            << " "  <<  wmo->DoodadVertices[i].Z
                            << " "  << -wmo->DoodadVertices[i].X << std::endl;

                    for (unsigned int i = 0; i < wmo->DoodadIndices.size(); i+=3)
                        out << "f " << indexOffset + wmo->DoodadIndices[i]   << " " 
                                    << indexOffset + wmo->DoodadIndices[i+1] << " "
                                    << indexOffset + wmo->DoodadIndices[i+2] << std::endl;

                    indexOffset += wmo->DoodadVertices.size();
                }

                // doodad vertices
                out << "# doodad vertices" << std::endl;
                for (auto id = chunk->m_doodads.begin(); id != chunk->m_doodads.end(); ++id)
                {
                    // if this wmo has already been dumped as part of another chunk, skip it
                    if (dumpedDoodads.find(*id) != dumpedDoodads.end())
                        continue;

                    dumpedDoodads.insert(*id);

                    Doodad *doodad = m_continent->GetDoodad(*id);

                    for (unsigned int i = 0; i < doodad->Vertices.size(); ++i)
                        out << "v " << -doodad->Vertices[i].Y
                            << " "  <<  doodad->Vertices[i].Z
                            << " "  << -doodad->Vertices[i].X << std::endl;

                    for (unsigned int i = 0; i < doodad->Indices.size(); i+=3)
                        out << "f " << indexOffset + doodad->Indices[i]   << " " 
                                    << indexOffset + doodad->Indices[i+1] << " "
                                    << indexOffset + doodad->Indices[i+2] << std::endl;

                    indexOffset += doodad->Vertices.size();
                }
            }

        out.close();
    }

    void Adt::SaveToDisk() const
    {
        const char zero = 0;
        std::stringstream ss;

        if (!Directory::Exists("Data"))
            Directory::Create("Data");

        if (!Directory::Exists(std::string("Data\\") + m_continent->Name))
            Directory::Create(std::string("Data\\") + m_continent->Name);

        if (!Directory::Exists(std::string("Data\\") + m_continent->Name + "\\ADTs"))
            Directory::Create(std::string("Data\\") + m_continent->Name + "\\ADTs");

        ss << "Data\\" << m_continent->Name << "\\ADTs\\" << X << "_" << Y << ".nadt";

        std::ofstream out(ss.str(), std::ios::out | std::ios::binary);

        out.write(VERSION, strlen(VERSION));
        out.write(&zero, 1);
        out.write("ADT", 3);

        char reservedSpaceForChunkOffsets[sizeof(unsigned int)*16*16];
        memset(reservedSpaceForChunkOffsets, 0, sizeof(int)*16*16);
        out.write(reservedSpaceForChunkOffsets, sizeof(int)*16*16);

        int indexOffset = 0;

        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 16; ++x)
            {
                unsigned int pos = (unsigned int)out.tellp();

                // move to chunk offset storage location for this chunk, and save it

                out.seekp(8+1+3+(y*16+x)*sizeof(unsigned int));
                out.write((const char *)&pos, sizeof(unsigned int));
                out.seekp(pos);

                AdtChunk *chunk = m_chunks[y][x].get();

                // label start of chunk
                out.write("CHUNK", 5);

                // write terrain vertex count
                const unsigned int terrainVertexCount = chunk->m_terrainVertices.size();
                out.write((const char *)&terrainVertexCount, sizeof(unsigned int));

                // write terrain index count
                const unsigned int terrainIndexCount = chunk->m_terrainIndices.size();
                out.write((const char *)&terrainIndexCount, sizeof(unsigned int));

                // write liquid vertex count
                const unsigned int liquidVertexCount = chunk->m_liquidVertices.size();
                out.write((const char *)&liquidVertexCount, sizeof(unsigned int));

                // write liquid index count
                const unsigned int liquidIndexCount = chunk->m_liquidIndices.size();
                out.write((const char *)&liquidIndexCount, sizeof(unsigned int));

                // write wmo unique id count
                const unsigned int wmoIdCount = chunk->m_wmos.size();
                out.write((const char *)&wmoIdCount, sizeof(unsigned int));

                // write doodad unique id count
                const unsigned int doodadIdCount = chunk->m_doodads.size();
                out.write((const char *)&doodadIdCount, sizeof(unsigned int));

                // write terrain heights
                out.write((const char *)&chunk->m_terrainHeights[0], sizeof(float)*145);

                // write hole map
                out.write((const char *)&chunk->m_holeMap[0], sizeof(bool)*8*8);

                if (terrainVertexCount && terrainIndexCount)
                {
                    // write terrain vertices
                    out.write((const char *)&chunk->m_terrainVertices[0], sizeof(Vertex)*terrainVertexCount);

                    // write terrain indices
                    out.write((const char *)&chunk->m_terrainIndices[0], sizeof(int)*terrainIndexCount);
                }

                if (liquidVertexCount && liquidIndexCount)
                {
                    // write liquid vertices
                    out.write((const char *)&chunk->m_liquidVertices[0], sizeof(Vertex)*liquidVertexCount);

                    // write liquid indices
                    out.write((const char *)&chunk->m_liquidIndices[0], sizeof(int)*liquidIndexCount);
                }

                // write wmo unique ids for this chunk
                for (auto i = chunk->m_wmos.begin(); i != chunk->m_wmos.end(); ++i)
                {
                    const unsigned int id = *i;
                    out.write((const char *)&id, sizeof(unsigned int));
                }

                // write doodad unique ids for this chunk
                for (auto i = chunk->m_doodads.begin(); i != chunk->m_doodads.end(); ++i)
                {
                    const unsigned int id = *i;
                    out.write((const char *)&id, sizeof(unsigned int));
                }
            }

        out.close();
    }

    const AdtChunk *Adt::GetChunk(const int chunkX, const int chunkY) const
    {
        return m_chunks[chunkX][chunkY].get();
    }
}