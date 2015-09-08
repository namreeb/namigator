#include <string>
#include <fstream>

#include "Output/Wmo.hpp"
#include "parser.hpp"

namespace parser
{
    Wmo::Wmo(std::vector<Vertex> &vertices, std::vector<int> &indices, std::vector<Vertex> &liquidVertices,
             std::vector<int> &liquidIndices, std::vector<Vertex> &doodadVertices, std::vector<int> &doodadIndices,
             const float minZ, const float maxZ) : MinZ(minZ), MaxZ(maxZ)
    {
        Vertices = std::move(vertices);
        Indices = std::move(indices);

        LiquidVertices = std::move(liquidVertices);
        LiquidIndices = std::move(liquidIndices);

        DoodadVertices = std::move(doodadVertices);
        DoodadIndices = std::move(doodadIndices);
    }

    void Wmo::SaveToDisk(const std::string &path)
    {
        const char zero = 0;
        std::ofstream out(path, std::ios::out | std::ios::binary);

        out.write(VERSION, strlen(VERSION));
        out.write(&zero, 1);
        out.write("WMO", 3);

        const unsigned int terrainVertexCount = Vertices.size();
        out.write((const char *)&terrainVertexCount, sizeof(unsigned int));

        const unsigned int terrainIndexCount = Indices.size();
        out.write((const char *)&terrainIndexCount, sizeof(unsigned int));

        const unsigned int liquidVertexCount = LiquidVertices.size();
        out.write((const char *)&liquidVertexCount, sizeof(unsigned int));

        const unsigned int liquidIndexCount = LiquidIndices.size();
        out.write((const char *)&liquidIndexCount, sizeof(unsigned int));

        const unsigned int doodadVertexCount = DoodadVertices.size();
        out.write((const char *)&doodadVertexCount, sizeof(unsigned int));

        const unsigned int doodadIndexCount = DoodadIndices.size();
        out.write((const char *)&doodadIndexCount, sizeof(unsigned int));

        out.write((const char *)&MinZ, sizeof(float));
        out.write((const char *)&MaxZ, sizeof(float));

        out.write((const char *)&Vertices[0], sizeof(Vertex) * terrainVertexCount);
        out.write((const char *)&Indices[0], sizeof(int) * terrainIndexCount);

        if (liquidVertexCount && liquidIndexCount)
        {
            out.write((const char *)&LiquidVertices[0], sizeof(Vertex) * liquidVertexCount);
            out.write((const char *)&LiquidIndices[0], sizeof(int) * liquidIndexCount);
        }

        if (doodadVertexCount && doodadIndexCount)
        {
            out.write((const char *)&DoodadVertices[0], sizeof(Vertex) * doodadVertexCount);
            out.write((const char *)&DoodadIndices[0], sizeof(int) * doodadIndexCount);
        }

        out.close();
    }
}