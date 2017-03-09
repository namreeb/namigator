#pragma once

#include "parser/Include/Doodad/Doodad.hpp"
#include "parser/Include/Adt/AdtChunkLocation.hpp"

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

        std::set<AdtChunkLocation> AdtChunks;

        const Doodad * const Model;

        DoodadInstance(const Doodad *doodad, const utility::Matrix &transformMatrix);
        //DoodadInstance(const Doodad *doodad, const utility::Vertex &position, const utility::Quaternion &rotation);

        utility::Vertex TransformVertex(const utility::Vertex &vertex) const;
        void BuildTriangles(std::vector<utility::Vertex> &vertices, std::vector<int> &indices) const;
};
}