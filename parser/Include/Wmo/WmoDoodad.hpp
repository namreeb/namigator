#pragma once

#include "parser/Include/Doodad/Doodad.hpp"

#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/BoundingBox.hpp"

#include <vector>
#include <memory>

namespace parser
{
class WmoDoodad
{
    public:
        // must be shared because it can also be owned by a map
        std::shared_ptr<const Doodad> Parent;

        const utility::Matrix TransformMatrix;
        utility::BoundingBox Bounds;

        WmoDoodad(std::shared_ptr<const Doodad> &doodad, const utility::Matrix &transformMatrix);

        utility::Vertex TransformVertex(const utility::Vertex &vertex) const;
        void BuildTriangles(std::vector<utility::Vertex> &vertices, std::vector<int> &indices) const;
};
}