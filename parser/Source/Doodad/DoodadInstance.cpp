#include "Doodad/DoodadInstance.hpp"
#include "Adt/AdtChunkLocation.hpp"

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
void UpdateBounds(utility::BoundingBox &bounds, const utility::Vertex &vertex, std::set<AdtChunkLocation> &adtChunks)
{
    bounds.update(vertex);

    int adtX, adtY, chunkX, chunkY;
    utility::Convert::WorldToAdt(vertex, adtX, adtY, chunkX, chunkY);
    adtChunks.insert({ static_cast<std::uint8_t>(adtX), static_cast<std::uint8_t>(adtY), static_cast<std::uint8_t>(chunkX), static_cast<std::uint8_t>(chunkY) });
}
}

DoodadInstance::DoodadInstance(const Doodad *doodad, const utility::Matrix &transformMatrix) : TransformMatrix(transformMatrix), Model(doodad)
{
    std::vector<utility::Vertex> vertices;
    std::vector<int> indices;

    BuildTriangles(vertices, indices);

    Bounds = { vertices[indices[0]], vertices[indices[0]] };

    for (auto const &v : vertices)
        UpdateBounds(Bounds, v, AdtChunks);
}

//DoodadInstance::DoodadInstance(const Doodad *doodad, const utility::Vertex &position, const utility::Quaternion &rotation) : TransformMatrix()
//{
//    
//}

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