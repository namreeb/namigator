#pragma once

#include "Common.hpp"
#include "utility/BoundingBox.hpp"
#include "utility/MathHelper.hpp"
#include "utility/Matrix.hpp"
#include "utility/Vector.hpp"

#include <cstdint>

namespace parser
{
namespace input
{
#pragma pack(push, 1)
struct WmoPlacement
{
    std::uint32_t NameId;
    std::uint32_t UniqueId;
    math::Vertex BasePosition;
    math::Vector3 Orientation;
    math::BoundingBox Bounds;
    std::uint16_t Flags;
    std::uint16_t DoodadSet;
    std::uint16_t NameSet;
    std::uint16_t _pad;

    void GetBoundingBox(math::BoundingBox& bounds) const
    {
        math::Vertex minCorner, maxCorner;

        if (UniqueId == 0xFFFFFFFF)
        {
            minCorner = {Bounds.MinCorner.Z, Bounds.MinCorner.X,
                         Bounds.MinCorner.Y};
            maxCorner = {Bounds.MaxCorner.Z, Bounds.MaxCorner.Y,
                         Bounds.MaxCorner.Y};
        }
        else
        {
            constexpr float mid = 32.f * MeshSettings::AdtSize;

            minCorner = {mid - Bounds.MaxCorner.Z, mid - Bounds.MaxCorner.X,
                         Bounds.MinCorner.Y};
            maxCorner = {mid - Bounds.MinCorner.Z, mid - Bounds.MinCorner.X,
                         Bounds.MaxCorner.Y};
        }

        bounds = math::BoundingBox(minCorner, maxCorner);
    }

    void GetTransformMatrix(math::Matrix& matrix) const
    {
        auto constexpr mid = 32.f * MeshSettings::AdtSize;

        // rotation around x (north/south)
        auto const rotX = math::Convert::ToRadians(Orientation.Z);

        // rotation around y (east/west)
        auto const rotY = math::Convert::ToRadians(Orientation.X);

        // rotation around z (vertical)
        auto const rotZ = math::Convert::ToRadians(Orientation.Y + 180.f);

        // 0xFFFFFFFF is the unique id used for a global WMO (a map which has no
        // ADTs but instead spawns a single WMO)
        const math::Matrix translationMatrix =
            UniqueId == 0xFFFFFFFF ?
                math::Matrix::CreateTranslationMatrix(
                    {BasePosition.Z, BasePosition.X, BasePosition.Y}) :
                math::Matrix::CreateTranslationMatrix({mid - BasePosition.Z,
                                                       mid - BasePosition.X,
                                                       BasePosition.Y});

        matrix = translationMatrix * math::Matrix::CreateRotationZ(rotZ) *
                 math::Matrix::CreateRotationY(rotY) *
                 math::Matrix::CreateRotationX(rotX);
    }
};
#pragma pack(pop)
} // namespace input
} // namespace parser