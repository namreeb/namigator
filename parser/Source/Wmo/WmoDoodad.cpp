#include "Wmo/WmoDoodad.hpp"

#include "utility/Include/LinearAlgebra.hpp"

#include <vector>
#include <algorithm>

namespace parser
{
WmoDoodad::WmoDoodad(const Doodad *doodad, const utility::Matrix &transformMatrix) : Parent(doodad), TransformMatrix(transformMatrix) {}

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