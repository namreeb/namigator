#pragma once

#include "utility/Include/AABBTree.hpp"

#include <memory>

namespace pathfind
{
struct DoodadInstance
{
    utility::Matrix m_transformMatrix;
    utility::Matrix m_inverseTransformMatrix;   // XXX FIXME Not implemented yet!
    utility::BoundingBox m_bounds;
    std::string m_fileName;
};

struct WmoInstance
{
    unsigned short m_doodadSet;
    utility::Matrix m_transformMatrix;
    utility::Matrix m_inverseTransformMatrix;   // XXX FIXME Not implemented yet!
    utility::BoundingBox m_bounds;
    std::string m_fileName;
};

// only loaded as needed
struct DoodadModel
{
    utility::AABBTree m_aabbTree;
};

// only loaded as needed
struct WmoModel
{
    utility::AABBTree m_aabbTree;
    std::vector<std::vector<DoodadInstance>> m_doodadSets;
    std::vector<std::vector<std::shared_ptr<DoodadModel>>> m_loadedDoodadSets;
};
}