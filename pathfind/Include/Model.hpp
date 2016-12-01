#pragma once

#include "utility/Include/AABBTree.hpp"

#include <memory>
#include <string>

namespace pathfind
{
// only loaded as needed
struct DoodadModel
{
    utility::AABBTree m_aabbTree;
};

// always loaded
struct DoodadInstance
{
    utility::Matrix m_transformMatrix;
    utility::Matrix m_inverseTransformMatrix;
    utility::BoundingBox m_bounds;
    std::string m_modelFilename;
    std::weak_ptr<DoodadModel> m_model;
};

// only loaded as needed
struct WmoModel
{
    utility::AABBTree m_aabbTree;
    std::vector<std::vector<DoodadInstance>> m_doodadSets;
    std::vector<std::vector<std::shared_ptr<DoodadModel>>> m_loadedDoodadSets;
};

// always loaded
struct WmoInstance
{
    unsigned short m_doodadSet;
    utility::Matrix m_transformMatrix;
    utility::Matrix m_inverseTransformMatrix;
    utility::BoundingBox m_bounds;
    std::string m_modelFilename;
    std::weak_ptr<WmoModel> m_model;
};
}