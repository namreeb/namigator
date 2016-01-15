#pragma once

#include "utility/Include/LinearAlgebra.hpp"

#include "DetourNavMesh.h"

#include <string>
#include <vector>

namespace pathfind
{
class NavMesh
{
    private:
        const std::string m_dataPath;
        const std::string m_continentName;

        dtNavMesh m_navMesh;

    public:
        NavMesh(const std::string &dataPath, const std::string &continentName);

        bool LoadTile(int x, int y);
        bool LoadGlobalWMO();

        void GetTileGeometry(int x, int y, std::vector<utility::Vertex> &vertices, std::vector<int> &indices);
};
}