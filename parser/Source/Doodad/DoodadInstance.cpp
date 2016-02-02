#include "Doodad/DoodadInstance.hpp"

#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/MathHelper.hpp"

#include <vector>
#include <string>
#include <fstream>
#include <algorithm>

namespace parser
{
namespace
{
    void UpdateBounds(utility::BoundingBox &bounds, const utility::Vertex &vertex)
    {
        bounds.MinCorner.X = std::min(bounds.MinCorner.X, vertex.X);
        bounds.MaxCorner.X = std::max(bounds.MaxCorner.X, vertex.X);
        bounds.MinCorner.Y = std::min(bounds.MinCorner.Y, vertex.Y);
        bounds.MaxCorner.Y = std::max(bounds.MaxCorner.Y, vertex.Y);
        bounds.MinCorner.Z = std::min(bounds.MinCorner.Z, vertex.Z);
        bounds.MaxCorner.Z = std::max(bounds.MaxCorner.Z, vertex.Z);
    }
}

DoodadInstance::DoodadInstance(const Doodad *doodad, const utility::Matrix &transformMatrix) : TransformMatrix(transformMatrix), Model(doodad)
{
    std::vector<utility::Vertex> vertices;
    std::vector<int> indices;

    BuildTriangles(vertices, indices);

    for (auto const &v : vertices)
    {
        UpdateBounds(Bounds, v);

        int adtX, adtY;
        utility::Convert::WorldToAdt(v, adtX, adtY);
        Adts.insert({ adtX, adtY });
    }
}

utility::Vertex DoodadInstance::TransformVertex(const utility::Vertex &vertex) const
{
    return utility::Vertex::Transform(vertex, TransformMatrix);
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
}