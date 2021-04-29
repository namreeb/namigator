#include "Doodad/DoodadInstance.hpp"

#include "Adt/AdtChunkLocation.hpp"
#include "utility/BoundingBox.hpp"
#include "utility/MathHelper.hpp"
#include "utility/Vector.hpp"

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

namespace parser
{
namespace
{
void UpdateBounds(math::BoundingBox& bounds, const math::Vertex& vertex,
                  std::set<AdtChunkLocation>& adtChunks)
{
    bounds.update(vertex);

    int adtX, adtY, chunkX, chunkY;
    math::Convert::WorldToAdt(vertex, adtX, adtY, chunkX, chunkY);
    adtChunks.insert(
        {static_cast<std::uint8_t>(adtX), static_cast<std::uint8_t>(adtY),
         static_cast<std::uint8_t>(chunkX), static_cast<std::uint8_t>(chunkY)});
}
} // namespace

DoodadInstance::DoodadInstance(const Doodad* doodad,
                               const math::Matrix& transformMatrix)
    : TransformMatrix(transformMatrix), Model(doodad)
{
    std::vector<math::Vertex> vertices;
    std::vector<int> indices;

    BuildTriangles(vertices, indices);

    Bounds = {vertices[indices[0]], vertices[indices[0]]};

    for (auto const& v : vertices)
        UpdateBounds(Bounds, v, AdtChunks);
}

// DoodadInstance::DoodadInstance(const Doodad *doodad, const math::Vector3
// &position, const math::Quaternion &rotation) : TransformMatrix()
//{
//
//}

math::Vertex DoodadInstance::TransformVertex(const math::Vertex& vertex) const
{
    return math::Vertex::Transform(vertex, TransformMatrix);
}

void DoodadInstance::BuildTriangles(std::vector<math::Vertex>& vertices,
                                    std::vector<int>& indices) const
{
    vertices.clear();
    vertices.reserve(Model->Vertices.size());

    indices.clear();
    indices.resize(Model->Indices.size());

    std::copy(Model->Indices.cbegin(), Model->Indices.cend(), indices.begin());

    for (auto& vertex : Model->Vertices)
        vertices.push_back(TransformVertex(vertex));
}
} // namespace parser