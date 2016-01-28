#pragma once

#include "utility/Include/LinearAlgebra.hpp"

namespace parser
{
namespace input
{
struct WmoDoodadPlacement
{
    unsigned int NameIndex;
    utility::Vertex Position;
    utility::Quaternion Orientation;
    float Scale;
    unsigned int Color;

    void GetTransformMatrix(utility::Matrix &matrix) const
    {
        matrix = utility::Matrix::CreateScalingMatrix(Scale) * utility::Matrix::CreateFromQuaternion(Orientation);
    }
};
}
}