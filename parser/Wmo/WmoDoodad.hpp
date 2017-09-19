#pragma once

#include "parser/Doodad/Doodad.hpp"

#include "utility/Vector.hpp"
#include "utility/Matrix.hpp"
#include "utility/BoundingBox.hpp"

#include <vector>
#include <memory>

namespace parser
{
class WmoDoodad
{
    public:
        // must be shared because it can also be owned by a map
        std::shared_ptr<const Doodad> Parent;

        const math::Matrix TransformMatrix;
        math::BoundingBox Bounds;

        WmoDoodad(std::shared_ptr<const Doodad> &doodad, const math::Matrix &transformMatrix);

        math::Vector3 TransformVertex(const math::Vector3 &vertex) const;
        void BuildTriangles(std::vector<math::Vector3> &vertices, std::vector<int> &indices) const;
};
}