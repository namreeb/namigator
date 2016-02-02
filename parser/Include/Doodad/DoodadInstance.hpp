#pragma once

#include "parser/Include/Doodad/Doodad.hpp"

#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/BoundingBox.hpp"

#include <vector>
#include <set>

namespace parser
{
class DoodadInstance
{
    public:
        const utility::Matrix TransformMatrix;
        utility::BoundingBox Bounds;

        std::set<std::pair<int, int>> Adts;

        const Doodad * const Model;

        DoodadInstance(const Doodad *doodad, const utility::Matrix &transformMatrix);

        utility::Vertex TransformVertex(const utility::Vertex &vertex) const;
        void BuildTriangles(std::vector<utility::Vertex> &vertices, std::vector<int> &indices) const;
};
}