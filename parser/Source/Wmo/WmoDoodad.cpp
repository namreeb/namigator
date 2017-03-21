#include "Wmo/WmoDoodad.hpp"

#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/Exception.hpp"

#include <vector>
#include <algorithm>
#include <memory>

namespace parser
{
WmoDoodad::WmoDoodad(std::shared_ptr<const Doodad> &doodad, const utility::Matrix &transformMatrix) : Parent(std::move(doodad)), TransformMatrix(transformMatrix)
{
    std::vector<utility::Vertex> vertices;
    std::vector<int> indices;

    BuildTriangles(vertices, indices);

    if (!vertices.size() || !indices.size())
        THROW("Empty WMO doodad instantiated");

    Bounds.MinCorner = Bounds.MaxCorner = vertices[0];

    for (size_t i = 1; i < vertices.size(); ++i)
    {
        Bounds.MinCorner.X = (std::min)(Bounds.MinCorner.X, vertices[i].X);
        Bounds.MaxCorner.X = (std::max)(Bounds.MaxCorner.X, vertices[i].X);
        Bounds.MinCorner.Y = (std::min)(Bounds.MinCorner.Y, vertices[i].Y);
        Bounds.MaxCorner.Y = (std::max)(Bounds.MaxCorner.Y, vertices[i].Y);
        Bounds.MinCorner.Z = (std::min)(Bounds.MinCorner.Z, vertices[i].Z);
        Bounds.MaxCorner.Z = (std::max)(Bounds.MaxCorner.Z, vertices[i].Z);
    }
}

utility::Vertex WmoDoodad::TransformVertex(const utility::Vertex &vertex) const
{
    return utility::Vertex::Transform(vertex, TransformMatrix);
}

void WmoDoodad::BuildTriangles(std::vector<utility::Vertex> &vertices, std::vector<int> &indices) const
{
    vertices.clear();
    vertices.reserve(Parent->Vertices.size());

    indices.clear();
    indices.resize(Parent->Indices.size());

    std::copy(Parent->Indices.cbegin(), Parent->Indices.cend(), indices.begin());

    for (auto &vertex : Parent->Vertices)
        vertices.push_back(TransformVertex(vertex));
}
}