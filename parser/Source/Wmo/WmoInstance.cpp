#include "Wmo/Wmo.hpp"
#include "Wmo/WmoInstance.hpp"

#include "utility/Include/MathHelper.hpp"

#include <string>
#include <sstream>
#include <fstream>
#include <set>

namespace parser
{
WmoInstance::WmoInstance(const Wmo *wmo, unsigned short doodadSet, const utility::BoundingBox &bounds, const utility::Vertex &origin, const utility::Matrix &transformMatrix)
    : Bounds(bounds), Origin(origin), TransformMatrix(transformMatrix), DoodadSet(doodadSet), Model(wmo)
{
    std::vector<utility::Vertex> vertices;
    std::vector<int> indices;

    BuildTriangles(vertices, indices);
    AmmendAdtSet(vertices);

    BuildLiquidTriangles(vertices, indices);
    AmmendAdtSet(vertices);

    BuildDoodadTriangles(vertices, indices);
    AmmendAdtSet(vertices);
}

void WmoInstance::AmmendAdtSet(const std::vector<utility::Vertex> &vertices)
{
    for (auto const &v : vertices)
    {
        int adtX, adtY;
        utility::Convert::WorldToAdt(v, adtX, adtY);
        Adts.insert({ adtX, adtY });
    }
}

utility::Vertex WmoInstance::TransformVertex(const utility::Vertex &vertex) const
{
    return Origin + utility::Vertex::Transform(vertex, TransformMatrix);
}

void WmoInstance::BuildTriangles(std::vector<utility::Vertex> &vertices, std::vector<int> &indices) const
{
    vertices.clear();
    vertices.reserve(Model->Vertices.size());

    indices.clear();
    indices.resize(Model->Indices.size());

    std::copy(Model->Indices.cbegin(), Model->Indices.cend(), indices.begin());

    for (auto &vertex : Model->Vertices)
        vertices.push_back(TransformVertex(vertex));
}

void WmoInstance::BuildLiquidTriangles(std::vector<utility::Vertex> &vertices, std::vector<int> &indices) const
{
    vertices.clear();
    vertices.reserve(Model->LiquidVertices.size());

    indices.clear();
    indices.resize(Model->LiquidIndices.size());

    std::copy(Model->LiquidIndices.cbegin(), Model->LiquidIndices.cend(), indices.begin());

    for (auto &vertex : Model->LiquidVertices)
        vertices.push_back(TransformVertex(vertex));
}

void WmoInstance::BuildDoodadTriangles(std::vector<utility::Vertex> &vertices, std::vector<int> &indices) const
{
    vertices.clear();
    indices.clear();

    size_t indexOffset = 0;

    auto const &doodadSet = Model->DoodadSets[DoodadSet];

    {
        size_t vertexCount = 0, indexCount = 0;

        // first, count how many vertiecs we will have.  this lets us do just one allocation.
        for (auto const &doodad : doodadSet)
        {
            vertexCount += doodad->Parent->Vertices.size();
            indexCount += doodad->Parent->Indices.size();
        }

        vertices.reserve(vertexCount);
        indices.reserve(indexCount);
    }

    for (auto const &doodad : doodadSet)
    {
        std::vector<utility::Vertex> doodadVertices;
        std::vector<int> doodadIndices;

        doodad->BuildTriangles(doodadVertices, doodadIndices);

        for (auto &vertex : doodadVertices)
            vertices.push_back(TransformVertex(vertex));

        for (auto i : doodadIndices)
            indices.push_back(indexOffset + i);

        indexOffset += doodadVertices.size();
    }
}

#ifdef _DEBUG
void WmoInstance::WriteGlobalObjFile(const std::string &MapName) const
{
    std::stringstream ss;

    ss << MapName << ".obj";

    std::ofstream out(ss.str());

    unsigned int indexOffset = 1;

    out << "# wmo vertices" << std::endl;

    std::vector<utility::Vertex> vertices;
    std::vector<int> indices;

    BuildTriangles(vertices, indices);

    for (auto const &vertex : vertices)
        out << "v " << -vertex.Y << " " << vertex.Z << " " << -vertex.X << std::endl;

    out << "# wmo indices" << std::endl;
    for (size_t i = 0; i < indices.size(); i += 3)
        out << "f " << indexOffset + indices[i + 0]
            << " "  << indexOffset + indices[i + 1]
            << " "  << indexOffset + indices[i + 2] << std::endl;

    indexOffset += vertices.size();

    out << "# wmo liquid vertices" << std::endl;

    BuildLiquidTriangles(vertices, indices);

    for (auto const &vertex : vertices)
        out << "v " << -vertex.Y << " " << vertex.Z << " " << -vertex.X << std::endl;

    out << "# wmo liquid incices" << std::endl;
    for (size_t i = 0; i < indices.size(); i += 3)
        out << "f " << indexOffset + indices[i + 0]
            << " "  << indexOffset + indices[i + 1]
            << " "  << indexOffset + indices[i + 2] << std::endl;

    indexOffset += vertices.size();

    out << "# wmo doodad vertices" << std::endl;

    BuildDoodadTriangles(vertices, indices);

    for (auto const &vertex : vertices)
        out << "v " << -vertex.Y << " " << vertex.Z << " " << -vertex.X << std::endl;

    out << "# wmo doodad indices" << std::endl;
    for (size_t i = 0; i < indices.size(); i += 3)
        out << "f " << indexOffset + indices[i + 0]
            << " "  << indexOffset + indices[i + 1]
            << " "  << indexOffset + indices[i + 2] << std::endl;

    out.close();
}

unsigned int WmoInstance::MemoryUsage() const
{
    return sizeof(WmoInstance);
}
#endif
}