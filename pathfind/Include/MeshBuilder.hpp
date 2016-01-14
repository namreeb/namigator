#pragma once

#include "utility/Include/LinearAlgebra.hpp"
#include "DataManager.hpp"

#include <vector>
#include <string>

namespace pathfind
{
namespace build
{
class MeshBuilder
{
    private:
        DataManager *const m_dataManager;

    public:
        MeshBuilder(DataManager *dataManager);

        bool GenerateAndSaveTile(int adtX, int adtY);
};
}
}