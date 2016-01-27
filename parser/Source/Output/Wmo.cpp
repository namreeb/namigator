#include "Output/Wmo.hpp"
#include "parser.hpp"

#include "utility/Include/MathHelper.hpp"

#include <string>
#include <sstream>
#include <fstream>
#include <set>

namespace parser
{
Wmo::Wmo(std::vector<utility::Vertex> &vertices, std::vector<int> &indices,
         std::vector<utility::Vertex> &liquidVertices, std::vector<int> &liquidIndices,
         std::vector<utility::Vertex> &doodadVertices, std::vector<int> &doodadIndices,
         const utility::BoundingBox &bounds)
    : Bounds(bounds),
      Vertices(std::move(vertices)), Indices(std::move(indices)),
      LiquidVertices(std::move(liquidVertices)), LiquidIndices(std::move(liquidIndices)),
      DoodadVertices(std::move(doodadVertices)), DoodadIndices(std::move(doodadIndices))
{
    AmmendAdtSet(Vertices);
    AmmendAdtSet(LiquidVertices);
    AmmendAdtSet(DoodadVertices);
}

void Wmo::AmmendAdtSet(const std::vector<utility::Vertex> &vertices)
{
    for (auto const &v : vertices)
    {
        int adtX, adtY;
        utility::Convert::WorldToAdt(v, adtX, adtY);
        Adts.insert({ adtX, adtY });
    }
}

#ifdef _DEBUG
void Wmo::WriteGlobalObjFile(const std::string &continentName) const
{
    std::stringstream ss;

    ss << continentName << ".obj";

    std::ofstream out(ss.str());

    unsigned int indexOffset = 1;

    out << "# wmo vertices" << std::endl;
    for (size_t i = 0; i < Vertices.size(); ++i)
        out << "v " << -Vertices[i].Y
            << " "  <<  Vertices[i].Z
            << " "  << -Vertices[i].X << std::endl;

    out << "# wmo indices" << std::endl;
    for (size_t i = 0; i < Indices.size(); i += 3)
        out << "f " << indexOffset + Indices[i + 0]
            << " "  << indexOffset + Indices[i + 1]
            << " "  << indexOffset + Indices[i + 2] << std::endl;

    indexOffset += Indices.size();

    out << "# wmo liquid vertices" << std::endl;
    for (size_t i = 0; i < LiquidVertices.size(); ++i)
        out << "v " << -LiquidVertices[i].Y
            << " "  <<  LiquidVertices[i].Z
            << " "  << -LiquidVertices[i].X << std::endl;

    out << "# wmo liquid incices" << std::endl;
    for (size_t i = 0; i < LiquidIndices.size(); i += 3)
        out << "f " << indexOffset + LiquidIndices[i + 0]
            << " "  << indexOffset + LiquidIndices[i + 1]
            << " "  << indexOffset + LiquidIndices[i + 2] << std::endl;

    indexOffset += LiquidIndices.size();

    out << "# wmo doodad vertices" << std::endl;
    for (size_t i = 0; i < DoodadVertices.size(); ++i)
        out << "v " << -DoodadVertices[i].Y
            << " "  <<  DoodadVertices[i].Z
            << " "  << -DoodadVertices[i].X << std::endl;

    out << "# wmo doodad indices" << std::endl;
    for (size_t i = 0; i < DoodadIndices.size(); i += 3)
        out << "f " << indexOffset + DoodadIndices[i + 0]
            << " "  << indexOffset + DoodadIndices[i + 1]
            << " "  << indexOffset + DoodadIndices[i + 2] << std::endl;

    out.close();
}

unsigned int Wmo::MemoryUsage() const
{
	unsigned int ret = sizeof(Wmo);

	ret += Vertices.size() * sizeof(utility::Vertex);
	ret += Indices.size() * sizeof(int);
	
	ret += LiquidVertices.size() * sizeof(utility::Vertex);
	ret += LiquidIndices.size() * sizeof(int);

	ret += DoodadVertices.size() * sizeof(utility::Vertex);
	ret += DoodadIndices.size() * sizeof(int);

	return ret;
}
#endif
}