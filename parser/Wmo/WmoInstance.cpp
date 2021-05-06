#include "Wmo/WmoInstance.hpp"

#include "Adt/AdtChunkLocation.hpp"
#include "Common.hpp"
#include "Wmo/Wmo.hpp"
#include "utility/BoundingBox.hpp"
#include "utility/MathHelper.hpp"
#include "utility/Matrix.hpp"
#include "utility/Vector.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#ifdef _DEBUG
#    include <iomanip>
#    include <iostream>
#endif

namespace parser
{
namespace
{
void UpdateBounds(math::BoundingBox& bounds, const math::Vertex& vertex,
                  std::set<AdtChunkLocation>& adtChunks)
{
    bounds.MinCorner.X = std::min(bounds.MinCorner.X, vertex.X);
    bounds.MaxCorner.X = std::max(bounds.MaxCorner.X, vertex.X);
    bounds.MinCorner.Y = std::min(bounds.MinCorner.Y, vertex.Y);
    bounds.MaxCorner.Y = std::max(bounds.MaxCorner.Y, vertex.Y);
    bounds.MinCorner.Z = std::min(bounds.MinCorner.Z, vertex.Z);
    bounds.MaxCorner.Z = std::max(bounds.MaxCorner.Z, vertex.Z);

    // some models can protrude beyond the map edge
    if (fabsf(vertex.X) > MeshSettings::MaxCoordinate ||
        fabsf(vertex.Y) > MeshSettings::MaxCoordinate)
        return;

    int adtX, adtY, chunkX, chunkY;
    math::Convert::WorldToAdt(vertex, adtX, adtY, chunkX, chunkY);
    adtChunks.insert(
        {static_cast<std::uint8_t>(adtX), static_cast<std::uint8_t>(adtY),
         static_cast<std::uint8_t>(chunkX), static_cast<std::uint8_t>(chunkY)});
}
} // namespace

WmoInstance::WmoInstance(const Wmo* wmo, unsigned int doodadSet,
                         unsigned int nameSet, const math::BoundingBox& bounds,
                         const math::Matrix& transformMatrix)
    : Bounds(bounds), TransformMatrix(transformMatrix), DoodadSet(doodadSet),
      NameSet(nameSet), Model(wmo)
{
    std::vector<math::Vector3> vertices;
    std::vector<int> indices;

    BuildTriangles(vertices, indices);

    for (auto const& v : vertices)
        UpdateBounds(Bounds, v, AdtChunks);

    BuildLiquidTriangles(vertices, indices);

    for (auto const& v : vertices)
        UpdateBounds(Bounds, v, AdtChunks);

    BuildDoodadTriangles(vertices, indices);

    for (auto const& v : vertices)
        UpdateBounds(Bounds, v, AdtChunks);
}

math::Vertex WmoInstance::TransformVertex(const math::Vertex& vertex) const
{
    return math::Vertex::Transform(vertex, TransformMatrix);
}

void WmoInstance::BuildTriangles(std::vector<math::Vertex>& vertices,
                                 std::vector<std::int32_t>& indices) const
{
    vertices.clear();
    vertices.reserve(Model->Vertices.size());

    indices.clear();
    indices.resize(Model->Indices.size());

    std::copy(Model->Indices.cbegin(), Model->Indices.cend(), indices.begin());

    for (auto& Vector3 : Model->Vertices)
        vertices.push_back(TransformVertex(Vector3));
}

void WmoInstance::BuildLiquidTriangles(std::vector<math::Vertex>& vertices,
                                       std::vector<std::int32_t>& indices) const
{
    vertices.clear();
    vertices.reserve(Model->LiquidVertices.size());

    indices.clear();
    indices.resize(Model->LiquidIndices.size());

    std::copy(Model->LiquidIndices.cbegin(), Model->LiquidIndices.cend(),
              indices.begin());

    for (auto& Vector3 : Model->LiquidVertices)
        vertices.push_back(TransformVertex(Vector3));
}

void WmoInstance::BuildDoodadTriangles(std::vector<math::Vertex>& vertices,
                                       std::vector<std::int32_t>& indices) const
{
    vertices.clear();
    indices.clear();

    size_t indexOffset = 0;

    // TODO: this happens in outlands (ADT 18, 37)
    if (DoodadSet >= Model->DoodadSets.size())
        return;

    auto const& doodadSet = Model->DoodadSets[DoodadSet];

    {
        size_t vertexCount = 0, indexCount = 0;

        // first, count how many vertices we will have.  this lets us do just
        // one allocation.
        for (auto const& doodad : doodadSet)
        {
            vertexCount += doodad->Parent->Vertices.size();
            indexCount += doodad->Parent->Indices.size();
        }

        vertices.reserve(vertexCount);
        indices.reserve(indexCount);
    }

    for (auto const& doodad : doodadSet)
    {
        std::vector<math::Vector3> doodadVertices;
        std::vector<int> doodadIndices;

        doodad->BuildTriangles(doodadVertices, doodadIndices);

        for (auto& Vector3 : doodadVertices)
            vertices.push_back(TransformVertex(Vector3));

        for (auto i : doodadIndices)
            indices.push_back(static_cast<std::int32_t>(indexOffset + i));

        indexOffset += doodadVertices.size();
    }
}
} // namespace parser