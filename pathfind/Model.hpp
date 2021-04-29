#pragma once

#include "utility/AABBTree.hpp"
#include "utility/BoundingBox.hpp"
#include "utility/Matrix.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace pathfind
{
struct Model
{
    math::AABBTree m_aabbTree;
};

// only loaded as needed
struct DoodadModel : Model
{
};

// always loaded
struct DoodadInstance
{
    math::Matrix m_transformMatrix;
    math::Matrix m_inverseTransformMatrix;
    math::BoundingBox m_bounds;
    std::string m_modelFilename;
    std::vector<math::Vertex>
        m_translatedVertices; // wow coordinate space.  indices are obtained
                              // from model.
    std::weak_ptr<DoodadModel> m_model;
};

// only loaded as needed
struct WmoModel : Model
{
    std::vector<std::vector<DoodadInstance>> m_doodadSets;
    std::vector<std::vector<std::shared_ptr<DoodadModel>>> m_loadedDoodadSets;
    std::unordered_map<unsigned int, std::pair<unsigned int, unsigned int>>
        m_nameSetToAreaZone;
};

// always loaded
struct WmoInstance
{
    unsigned int m_doodadSet;
    unsigned int m_nameSet;
    math::Matrix m_transformMatrix;
    math::Matrix m_inverseTransformMatrix;
    math::BoundingBox m_bounds;
    std::string m_modelFilename;
    std::weak_ptr<WmoModel> m_model;
};
} // namespace pathfind