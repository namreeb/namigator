#pragma once

#include "parser/Adt/AdtChunkLocation.hpp"
#include "parser/Wmo/Wmo.hpp"
#include "utility/BoundingBox.hpp"
#include "utility/Matrix.hpp"
#include "utility/Vector.hpp"

#include <cstdint>
#include <set>
#include <vector>

namespace parser
{
class WmoInstance
{
public:
    const math::Matrix TransformMatrix;
    math::BoundingBox Bounds;

    const Wmo* const Model;

    std::set<AdtChunkLocation> AdtChunks;

    const unsigned int DoodadSet;
    const unsigned int NameSet;

    WmoInstance(const Wmo* wmo, unsigned int doodadSet, unsigned int nameSet,
                const math::BoundingBox& bounds,
                const math::Matrix& transformMatrix);

    math::Vertex TransformVertex(const math::Vertex& vertex) const;
    void BuildTriangles(std::vector<math::Vertex>& vertices,
                        std::vector<std::int32_t>& indices) const;
    void BuildLiquidTriangles(std::vector<math::Vertex>& vertices,
                              std::vector<std::int32_t>& indices) const;
    void BuildDoodadTriangles(std::vector<math::Vertex>& vertices,
                              std::vector<std::int32_t>& indices)
        const; // note: this assembles the triangles from all doodads in this
               // wmo into one collection
};
} // namespace parser