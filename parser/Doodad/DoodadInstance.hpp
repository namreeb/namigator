#pragma once

#include "parser/Doodad/Doodad.hpp"
#include "parser/Adt/AdtChunkLocation.hpp"

#include "utility/Matrix.hpp"
#include "utility/Vector.hpp"
#include "utility/BoundingBox.hpp"

#include <vector>
#include <set>

namespace parser
{
class DoodadInstance
{
    public:
        const math::Matrix TransformMatrix;
        math::BoundingBox Bounds;

        std::set<AdtChunkLocation> AdtChunks;

        const Doodad * const Model;

        DoodadInstance(const Doodad *doodad, const math::Matrix &transformMatrix);
        //DoodadInstance(const Doodad *doodad, const utility::Vertex &position, const utility::Quaternion &rotation);

        math::Vector3 TransformVertex(const math::Vector3 &vertex) const;
        void BuildTriangles(std::vector<math::Vector3> &vertices, std::vector<int> &indices) const;
};
}