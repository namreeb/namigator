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
        const std::string m_MapName;

        dtNavMesh m_navMesh;

        static bool LoadFile(const std::string &filename, unsigned char **data, int *size);

    public:
        NavMesh(const std::string &dataPath, const std::string &MapName);

        bool LoadTile(int x, int y);
        bool LoadGlobalWMO();

        void GetTileGeometry(int x, int y, std::vector<utility::Vertex> &vertices, std::vector<int> &indices);
};
}