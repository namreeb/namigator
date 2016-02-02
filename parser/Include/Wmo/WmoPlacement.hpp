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
    utility::BoundingBox Bounds;
    std::uint16_t Flags;
    std::uint16_t DoodadSet;
    std::uint16_t NameSet;
    std::uint16_t _pad;

    void GetBoundingBox(utility::BoundingBox &bounds) const
    {
        utility::Vertex minCorner, maxCorner;

        if (UniqueId == 0xFFFFFFFF)
        {
            minCorner = { Bounds.MinCorner.Z, Bounds.MinCorner.X, Bounds.MinCorner.Y };
            maxCorner = { Bounds.MaxCorner.Z, Bounds.MaxCorner.Y, Bounds.MaxCorner.Y };
        }
        else
        {
            constexpr float mid = 32.f * utility::MathHelper::AdtSize;

            minCorner = { mid - Bounds.MaxCorner.Z, mid - Bounds.MaxCorner.X, Bounds.MinCorner.Y };
            maxCorner = { mid - Bounds.MinCorner.Z, mid - Bounds.MinCorner.X, Bounds.MaxCorner.Y };
        }

        bounds = utility::BoundingBox(minCorner, maxCorner);
    }

    void GetTransformMatrix(utility::Matrix &matrix) const
    {
        constexpr float mid = 32.f * utility::MathHelper::AdtSize;

        const float rotX = utility::Convert::ToRadians(Orientation.Z);
        const float rotY = utility::Convert::ToRadians(Orientation.X);
        const float rotZ = utility::Convert::ToRadians(Orientation.Y + 180.f);

        auto const translationMatrix = UniqueId == 0xFFFFFFFF ?
            utility::Matrix::CreateTranslationMatrix({ BasePosition.Z, BasePosition.X, BasePosition.Y }) :
            utility::Matrix::CreateTranslationMatrix({ mid - BasePosition.Z, mid - BasePosition.X, BasePosition.Y });

        matrix = translationMatrix *
                 utility::Matrix::CreateRotationX(rotX) *
                 utility::Matrix::CreateRotationY(rotY) *
                 utility::Matrix::CreateRotationZ(rotZ);
    }
};
}
}