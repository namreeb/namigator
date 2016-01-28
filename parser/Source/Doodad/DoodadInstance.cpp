#include "Doodad/DoodadInstance.hpp"

#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/MathHelper.hpp"

#include <vector>
#include <string>
#include <fstream>
#include <algorithm>

namespace parser
{
DoodadInstance::DoodadInstance(const Doodad *doodad, const utility::Vertex &origin, const utility::Matrix &transformMatrix)
    : Origin(origin), TransformMatrix(transformMatrix), Model(doodad)
{
    std::vector<utility::Vertex> vertices;
    std::vector<int> indices;

    BuildTriangles(vertices, indices);

    MinZ = vertices[0].Z;
    MaxZ = vertices[0].Z;

    for (auto const &v : vertices)
    {
        int adtX, adtY;
        utility::Convert::WorldToAdt(v, adtX, adtY);
        Adts.insert({ adtX, adtY });

        MaxZ = std::max(MaxZ, v.Z);
        MinZ = std::min(MinZ, v.Z);
    }
}

utility::Vertex DoodadInstance::TransformVertex(const utility::Vertex &vertex) const
{
    return Origin + utility::Vertex::Transform(vertex, TransformMatrix);
}

void DoodadInstance::BuildTriangles(std::vector<utility::Vertex> &vertices, std::vector<int> &indices) const
{
    vertices.clear();
    vertices.reserve(Model->Vertices.size());

    indices.clear();
    indices.resize(Model->Indices.size());

    std::copy(Model->Indices.cbegin(), Model->Indices.cend(), indices.begin());

    for (auto &vertex : Model->Vertices)
        vertices.push_back(TransformVertex(vertex));
}

#ifdef _DEBUG
unsigned int DoodadInstance::MemoryUsage() const
{
    return sizeof(DoodadInstance);
}
#endif
}