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
        //DoodadInstance(const Doodad *doodad, const math::Vertex &position, const math::Quaternion &rotation);

        math::Vertex TransformVertex(const math::Vertex& vertex) const;
        void BuildTriangles(std::vector<math::Vertex>& vertices, std::vector<int>& indices) const;
};
}