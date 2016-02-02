#pragma once

#include "parser/Include/Doodad/Doodad.hpp"

#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/BoundingBox.hpp"

#include <vector>

namespace parser
{
class WmoDoodad
{
    public:
        const Doodad * const Parent;
        const utility::Matrix TransformMatrix;
        utility::BoundingBox Bounds;

        WmoDoodad(const Doodad *doodad, const utility::Matrix &transformMatrix);

        utility::Vertex TransformVertex(const utility::Vertex &vertex) const;
        void BuildTriangles(std::vector<utility::Vertex> &vertices, std::vector<int> &indices) const;
};
}