#pragma once

#include "parser/Include/Wmo/Wmo.hpp"

#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/BoundingBox.hpp"

#include <vector>
#include <string>
#include <set>

namespace parser
{
class WmoInstance
{
    private:
        void AmmendAdtSet(const std::vector<utility::Vertex> &vertices);

    public:
        const utility::BoundingBox Bounds;
        const utility::Vertex Origin;
        const utility::Matrix TransformMatrix;
        const unsigned short DoodadSet;

        const Wmo * const Model;

        std::set<std::pair<int, int>> Adts;

        WmoInstance(const Wmo *wmo, unsigned short doodadSet, const utility::BoundingBox &bounds, const utility::Vertex &origin, const utility::Matrix &transformMatrix);

        utility::Vertex TransformVertex(const utility::Vertex &vertex) const;
        void BuildTriangles(std::vector<utility::Vertex> &vertices, std::vector<int> &indices) const;
        void BuildLiquidTriangles(std::vector<utility::Vertex> &vertices, std::vector<int> &indices) const;
        void BuildDoodadTriangles(std::vector<utility::Vertex> &vertices, std::vector<int> &indices) const;     // note: this assembles the triangles from all doodads in this wmo into one collection

#ifdef _DEBUG
        void WriteGlobalObjFile(const std::string &MapName) const;

        unsigned int MemoryUsage() const;
#endif
};
}