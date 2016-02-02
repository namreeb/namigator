#pragma once

#include "parser/Include/Doodad/Doodad.hpp"

#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/BoundingBox.hpp"

#include <vector>
#include <string>
#include <set>

namespace parser
{
class DoodadInstance
{
    public:
        const utility::Matrix TransformMatrix;

        std::set<std::pair<int, int>> Adts;

        const Doodad * const Model;

        float MinZ;
        float MaxZ;

        DoodadInstance(const Doodad *doodad, const utility::Matrix &transformMatrix);

        utility::Vertex TransformVertex(const utility::Vertex &vertex) const;
        void BuildTriangles(std::vector<utility::Vertex> &vertices, std::vector<int> &indices) const;

#ifdef _DEBUG
        unsigned int MemoryUsage() const;
#endif
};
}