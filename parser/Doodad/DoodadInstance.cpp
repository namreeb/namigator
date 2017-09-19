#include "Doodad/DoodadInstance.hpp"
#include "Adt/AdtChunkLocation.hpp"

#include "utility/BoundingBox.hpp"
#include "utility/Vector.hpp"
#include "utility/MathHelper.hpp"

#include <vector>
#include <string>
#include <fstream>
#include <algorithm>

namespace parser
{
namespace
{
void UpdateBounds(math::BoundingBox &bounds, const math::Vector3 &Vector3, std::set<AdtChunkLocation> &adtChunks)
{
    bounds.update(Vector3);

    int adtX, adtY, chunkX, chunkY;
    math::Convert::WorldToAdt(Vector3, adtX, adtY, chunkX, chunkY);
    adtChunks.insert({ static_cast<std::uint8_t>(adtX), static_cast<std::uint8_t>(adtY), static_cast<std::uint8_t>(chunkX), static_cast<std::uint8_t>(chunkY) });
}
}

DoodadInstance::DoodadInstance(const Doodad *doodad, const math::Matrix &transformMatrix) : TransformMatrix(transformMatrix), Model(doodad)
{
    std::vector<math::Vector3> vertices;
    std::vector<int> indices;

    BuildTriangles(vertices, indices);

    Bounds = { vertices[indices[0]], vertices[indices[0]] };

    for (auto const &v : vertices)
        UpdateBounds(Bounds, v, AdtChunks);
}

//DoodadInstance::DoodadInstance(const Doodad *doodad, const math::Vector3 &position, const utility::Quaternion &rotation) : TransformMatrix()
//{
//    
//}

math::Vector3 DoodadInstance::TransformVertex(const math::Vector3 &Vector3) const
{
    return math::Vector3::Transform(Vector3, TransformMatrix);
}

void DoodadInstance::BuildTriangles(std::vector<math::Vector3> &vertices, std::vector<int> &indices) const
{
    vertices.clear();
    vertices.reserve(Model->Vertices.size());

    indices.clear();
    indices.resize(Model->Indices.size());

    std::copy(Model->Indices.cbegin(), Model->Indices.cend(), indices.begin());

    for (auto &Vector3 : Model->Vertices)
        vertices.push_back(TransformVertex(Vector3));
}
}