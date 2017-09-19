#include "Wmo/Wmo.hpp"
#include "Wmo/WmoInstance.hpp"
#include "Adt/AdtChunkLocation.hpp"

#include "utility/Vector.hpp"
#include "utility/Matrix.hpp"
#include "utility/MathHelper.hpp"
#include "utility/BoundingBox.hpp"

#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <set>

#ifdef _DEBUG
#include <iomanip>
#include <iostream>
#endif

namespace parser
{
namespace
{
void UpdateBounds(math::BoundingBox &bounds, const math::Vector3 &Vector3, std::set<AdtChunkLocation> &adtChunks)
{
    bounds.MinCorner.X = std::min(bounds.MinCorner.X, Vector3.X);
    bounds.MaxCorner.X = std::max(bounds.MaxCorner.X, Vector3.X);
    bounds.MinCorner.Y = std::min(bounds.MinCorner.Y, Vector3.Y);
    bounds.MaxCorner.Y = std::max(bounds.MaxCorner.Y, Vector3.Y);
    bounds.MinCorner.Z = std::min(bounds.MinCorner.Z, Vector3.Z);
    bounds.MaxCorner.Z = std::max(bounds.MaxCorner.Z, Vector3.Z);

    int adtX, adtY, chunkX, chunkY;
    math::Convert::WorldToAdt(Vector3, adtX, adtY, chunkX, chunkY);
    adtChunks.insert({ static_cast<std::uint8_t>(adtX), static_cast<std::uint8_t>(adtY), static_cast<std::uint8_t>(chunkX), static_cast<std::uint8_t>(chunkY) });
}
}

WmoInstance::WmoInstance(const Wmo *wmo, unsigned short doodadSet, const math::BoundingBox &bounds, const math::Matrix &transformMatrix) : Bounds(bounds), TransformMatrix(transformMatrix), DoodadSet(doodadSet), Model(wmo)
{
    std::vector<math::Vector3> vertices;
    std::vector<int> indices;

    BuildTriangles(vertices, indices);

    for (auto const &v : vertices)
        UpdateBounds(Bounds, v, AdtChunks);

    BuildLiquidTriangles(vertices, indices);

    for (auto const &v : vertices)
        UpdateBounds(Bounds, v, AdtChunks);

    BuildDoodadTriangles(vertices, indices);

    for (auto const &v : vertices)
        UpdateBounds(Bounds, v, AdtChunks);
}

math::Vector3 WmoInstance::TransformVertex(const math::Vector3 &Vector3) const
{
    return math::Vector3::Transform(Vector3, TransformMatrix);
}

void WmoInstance::BuildTriangles(std::vector<math::Vector3> &vertices, std::vector<std::int32_t> &indices) const
{
    vertices.clear();
    vertices.reserve(Model->Vertices.size());

    indices.clear();
    indices.resize(Model->Indices.size());

    std::copy(Model->Indices.cbegin(), Model->Indices.cend(), indices.begin());

    for (auto &Vector3 : Model->Vertices)
        vertices.push_back(TransformVertex(Vector3));
}

void WmoInstance::BuildLiquidTriangles(std::vector<math::Vector3> &vertices, std::vector<std::int32_t> &indices) const
{
    vertices.clear();
    vertices.reserve(Model->LiquidVertices.size());

    indices.clear();
    indices.resize(Model->LiquidIndices.size());

    std::copy(Model->LiquidIndices.cbegin(), Model->LiquidIndices.cend(), indices.begin());

    for (auto &Vector3 : Model->LiquidVertices)
        vertices.push_back(TransformVertex(Vector3));
}

void WmoInstance::BuildDoodadTriangles(std::vector<math::Vector3> &vertices, std::vector<std::int32_t> &indices) const
{
    vertices.clear();
    indices.clear();

    size_t indexOffset = 0;

    auto const &doodadSet = Model->DoodadSets[DoodadSet];

    {
        size_t Vector3Count = 0, indexCount = 0;

        // first, count how many vertices we will have.  this lets us do just one allocation.
        for (auto const &doodad : doodadSet)
        {
            Vector3Count += doodad->Parent->Vertices.size();
            indexCount += doodad->Parent->Indices.size();
        }

        vertices.reserve(Vector3Count);
        indices.reserve(indexCount);
    }

    for (auto const &doodad : doodadSet)
    {
        std::vector<math::Vector3> doodadVertices;
        std::vector<int> doodadIndices;

        doodad->BuildTriangles(doodadVertices, doodadIndices);

        for (auto &Vector3 : doodadVertices)
            vertices.push_back(TransformVertex(Vector3));

        for (auto i : doodadIndices)
            indices.push_back(static_cast<std::int32_t>(indexOffset + i));

        indexOffset += doodadVertices.size();
    }
}
}