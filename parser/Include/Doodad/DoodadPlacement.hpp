#pragma once

#include "utility/Include/MathHelper.hpp"
#include "utility/Include/LinearAlgebra.hpp"

#include <string>
#include <algorithm>
#include <vector>
#include <cstdint>

namespace parser
{
namespace input
{
struct DoodadPlacement
{
    std::uint32_t NameId;
    std::uint32_t UniqueId;
    utility::Vertex BasePosition;
    utility::Vector3 Orientation;
    std::uint16_t Scale;
    std::uint16_t Flags;

    void GetOrigin(utility::Vertex &vertex) const
    {
        constexpr float mid = 32.f * utility::MathHelper::AdtSize;

        vertex = utility::Vertex(mid - BasePosition.Z, mid - BasePosition.X, BasePosition.Y);
    }

    void GetTransformMatrix(utility::Matrix &matrix) const
    {
        const float rotX = utility::Convert::ToRadians(Orientation.Z);
        const float rotY = utility::Convert::ToRadians(Orientation.X);
        const float rotZ = utility::Convert::ToRadians(Orientation.Y + 180.f);

        matrix = utility::Matrix::CreateScalingMatrix(Scale / 1024.f) * utility::Matrix::CreateRotationZ(rotZ) * utility::Matrix::CreateRotationY(rotY) * utility::Matrix::CreateRotationX(rotX);
    }
};
}
}