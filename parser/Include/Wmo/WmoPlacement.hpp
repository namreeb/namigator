#pragma once

#include "utility/Include/MathHelper.hpp"
#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/BoundingBox.hpp"

#include <cstdint>

namespace parser
{
namespace input
{
struct WmoPlacement
{
    std::uint32_t NameId;
    std::uint32_t UniqueId;
    utility::Vertex BasePosition;
    utility::Vector3 Orientation;
    utility::Vertex MaxCorner;
    utility::Vertex MinCorner;
    std::uint16_t Flags;
    std::uint16_t DoodadSet;
    std::uint16_t NameSet;
    std::uint16_t _pad;

    void GetBoundingBox(utility::BoundingBox &bounds) const
    {
        constexpr float mid = 32.f * utility::MathHelper::AdtSize;

        const utility::Vertex maxCorner(mid - MaxCorner.Z, mid - MaxCorner.X, MaxCorner.Y);
        const utility::Vertex minCorner(mid - MinCorner.Z, mid - MinCorner.X, MinCorner.Y);

        bounds = utility::BoundingBox(minCorner, maxCorner);
    }

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

        matrix = utility::Matrix::CreateRotationX(rotX) * utility::Matrix::CreateRotationY(rotY) * utility::Matrix::CreateRotationZ(rotZ);
    }
};
}
}