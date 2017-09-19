#pragma once

#include "utility/Vector.hpp"
#include "utility/Matrix.hpp"
#include "utility/Quaternion.hpp"

#include <cstdint>

namespace parser
{
namespace input
{
#pragma pack(push, 1)
struct WmoDoodadPlacement
{
    std::uint32_t NameIndex;
    math::Vector3 Position;
    math::Quaternion Orientation;
    float Scale;
    std::uint32_t Color;

    void GetTransformMatrix(math::Matrix &matrix) const
    {
        matrix = math::Matrix::CreateTranslationMatrix(Position) * math::Matrix::CreateScalingMatrix(Scale) * math::Matrix::CreateFromQuaternion(Orientation);
    }
};
#pragma pack(pop)
}
}