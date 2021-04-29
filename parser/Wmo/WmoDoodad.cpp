#include "Wmo/WmoDoodad.hpp"

#include "utility/Exception.hpp"
#include "utility/Matrix.hpp"
#include "utility/Vector.hpp"

#include <algorithm>
#include <memory>
#include <vector>

namespace parser
{
WmoDoodad::WmoDoodad(std::shared_ptr<const Doodad>& doodad,
                     const math::Matrix& transformMatrix)
    : Parent(std::move(doodad)), TransformMatrix(transformMatrix)
{
    std::vector<math::Vector3> vertices;
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

math::Vector3 WmoDoodad::TransformVertex(const math::Vector3& Vector3) const
{
    return math::Vector3::Transform(Vector3, TransformMatrix);
}

void WmoDoodad::BuildTriangles(std::vector<math::Vector3>& vertices,
                               std::vector<int>& indices) const
{
    vertices.clear();
    vertices.reserve(Parent->Vertices.size());

    indices.clear();
    indices.resize(Parent->Indices.size());

    std::copy(Parent->Indices.cbegin(), Parent->Indices.cend(),
              indices.begin());

    for (auto& Vector3 : Parent->Vertices)
        vertices.push_back(TransformVertex(Vector3));
}
} // namespace parser