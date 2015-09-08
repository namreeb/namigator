#include <vector>
#include <string>
#include <fstream>

#include "parser.hpp"
#include "LinearAlgebra.hpp"
#include "Output/Doodad.hpp"

namespace parser
{
    Doodad::Doodad(std::vector<Vertex> &vertices, std::vector<unsigned short> &indices, const float minZ, const float maxZ) :
        MinZ(minZ), MaxZ(maxZ)
    {
        Vertices = std::move(vertices);
        Indices = std::move(indices);
    }

    void Doodad::SaveToDisk(const std::string &path) const
    {
        const char zero = 0;
        std::ofstream out(path, std::ios::out | std::ios::binary);

        out.write(VERSION, strlen(VERSION));
        out.write(&zero, 1);
        out.write("DOO", 3);

        const unsigned int terrainVertexCount = Vertices.size();
        out.write((const char *)&terrainVertexCount, sizeof(unsigned int));

        const unsigned int terrainIndexCount = Indices.size();
        out.write((const char *)&terrainIndexCount, sizeof(unsigned int));

        out.write((const char *)&MinZ, sizeof(float));
        out.write((const char *)&MaxZ, sizeof(float));

        out.write((const char *)&Vertices[0], sizeof(Vertex) * terrainVertexCount);
        out.write((const char *)&Indices[0], sizeof(unsigned short) * terrainIndexCount);

        out.close();
    }
}