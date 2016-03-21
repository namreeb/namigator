#pragma once

#include "parser/Include/Wmo/Wmo.hpp"
#include "parser/Include/Adt/AdtChunkLocation.hpp"

#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/BoundingBox.hpp"

#include <vector>
#include <string>
#include <set>
#include <cstdint>

namespace parser
{
class WmoInstance
{
    public:
        const utility::Matrix TransformMatrix;
        utility::BoundingBox Bounds;

        const Wmo * const Model;

        std::set<AdtChunkLocation> AdtChunks;

        const unsigned short DoodadSet;

        WmoInstance(const Wmo *wmo, unsigned short doodadSet, const utility::BoundingBox &bounds, const utility::Matrix &transformMatrix);

        utility::Vertex TransformVertex(const utility::Vertex &vertex) const;
        void BuildTriangles(std::vector<utility::Vertex> &vertices, std::vector<std::int32_t> &indices) const;
        void BuildLiquidTriangles(std::vector<utility::Vertex> &vertices, std::vector<std::int32_t> &indices) const;
        void BuildDoodadTriangles(std::vector<utility::Vertex> &vertices, std::vector<std::int32_t> &indices) const;    // note: this assembles the triangles from all doodads in this wmo into one collection
};
}