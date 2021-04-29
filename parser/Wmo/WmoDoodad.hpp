#pragma once

#include "parser/Doodad/Doodad.hpp"
#include "utility/BoundingBox.hpp"
#include "utility/Matrix.hpp"
#include "utility/Vector.hpp"

#include <memory>
#include <vector>

namespace parser
{
class WmoDoodad
{
public:
    // must be shared because it can also be owned by a map
    std::shared_ptr<const Doodad> Parent;

    const math::Matrix TransformMatrix;
    math::BoundingBox Bounds;

    WmoDoodad(std::shared_ptr<const Doodad>& doodad,
              const math::Matrix& transformMatrix);

    math::Vertex TransformVertex(const math::Vertex& vertex) const;
    void BuildTriangles(std::vector<math::Vertex>& vertices,
                        std::vector<int>& indices) const;
};
} // namespace parser