#pragma once

#include "utility/Include/LinearAlgebra.hpp"

#include <cstdint>

namespace parser
{
namespace input
{
struct WmoDoodadPlacement
{
    std::uint32_t NameIndex;
    utility::Vertex Position;
    utility::Quaternion Orientation;
    float Scale;
    std::uint32_t Color;

    void GetTransformMatrix(utility::Matrix &matrix) const
    {
        matrix = utility::Matrix::CreateTranslationMatrix(Position) * utility::Matrix::CreateScalingMatrix(Scale) * utility::Matrix::CreateFromQuaternion(Orientation);
    }
};
}
}