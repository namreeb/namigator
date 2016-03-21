#include "Wmo/Wmo.hpp"
#include "Wmo/WmoInstance.hpp"
#include "Adt/AdtChunkLocation.hpp"

#include "utility/Include/MathHelper.hpp"
#include "utility/Include/LinearAlgebra.hpp"

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
    void UpdateBounds(utility::BoundingBox &bounds, const utility::Vertex &vertex, std::set<AdtChunkLocation> &adtChunks)
    {
        bounds.MinCorner.X = std::min(bounds.MinCorner.X, vertex.X);
        bounds.MaxCorner.X = std::max(bounds.MaxCorner.X, vertex.X);
        bounds.MinCorner.Y = std::min(bounds.MinCorner.Y, vertex.Y);
        bounds.MaxCorner.Y = std::max(bounds.MaxCorner.Y, vertex.Y);
        bounds.MinCorner.Z = std::min(bounds.MinCorner.Z, vertex.Z);
        bounds.MaxCorner.Z = std::max(bounds.MaxCorner.Z, vertex.Z);

        int adtX, adtY, chunkX, chunkY;
        utility::Convert::WorldToAdt(vertex, adtX, adtY, chunkX, chunkY);
        adtChunks.insert({ static_cast<std::uint8_t>(adtX), static_cast<std::uint8_t>(adtY), static_cast<std::uint8_t>(chunkX), static_cast<std::uint8_t>(chunkY) });
    }
}

WmoInstance::WmoInstance(const Wmo *wmo, unsigned short doodadSet, const utility::BoundingBox &bounds, const utility::Matrix &transformMatrix) : Bounds(bounds), TransformMatrix(transformMatrix), DoodadSet(doodadSet), Model(wmo)
{
    std::vector<utility::Vertex> vertices;
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

utility::Vertex WmoInstance::TransformVertex(const utility::Vertex &vertex) const
{
    return utility::Vertex::Transform(vertex, TransformMatrix);
}

void WmoInstance::BuildTriangles(std::vector<utility::Vertex> &vertices, std::vector<std::int32_t> &indices) const
{
    vertices.clear();
    vertices.reserve(Model->Vertices.size());

    indices.clear();
    indices.resize(Model->Indices.size());

    std::copy(Model->Indices.cbegin(), Model->Indices.cend(), indices.begin());

    for (auto &vertex : Model->Vertices)
        vertices.push_back(TransformVertex(vertex));
}

void WmoInstance::BuildLiquidTriangles(std::vector<utility::Vertex> &vertices, std::vector<std::int32_t> &indices) const
{
    vertices.clear();
    vertices.reserve(Model->LiquidVertices.size());

    indices.clear();
    indices.resize(Model->LiquidIndices.size());

    std::copy(Model->LiquidIndices.cbegin(), Model->LiquidIndices.cend(), indices.begin());

    for (auto &vertex : Model->LiquidVertices)
        vertices.push_back(TransformVertex(vertex));
}

void WmoInstance::BuildDoodadTriangles(std::vector<utility::Vertex> &vertices, std::vector<std::int32_t> &indices) const
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
            indices.push_back(static_cast<std::int32_t>(indexOffset + i));

        indexOffset += doodadVertices.size();
    }
}
}