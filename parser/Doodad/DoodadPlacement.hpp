#pragma once

#include "Common.hpp"
#include "utility/MathHelper.hpp"
#include "utility/Matrix.hpp"
#include "utility/Vector.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace parser
{
namespace input
{
#pragma pack(push, 1)
struct DoodadPlacement
{
    std::uint32_t NameId;
    std::uint32_t UniqueId;
    math::Vertex BasePosition;
    math::Vertex Orientation;
    std::uint16_t Scale;
    std::uint16_t Flags;

    void GetTransformMatrix(math::Matrix& matrix) const
    {
        auto constexpr mid = 32.f * MeshSettings::AdtSize;

        auto const rotX = math::Convert::ToRadians(Orientation.Z);
        auto const rotY = math::Convert::ToRadians(Orientation.X);
        auto const rotZ = math::Convert::ToRadians(Orientation.Y + 180.f);

        matrix =
            math::Matrix::CreateTranslationMatrix(
                {mid - BasePosition.Z, mid - BasePosition.X, BasePosition.Y}) *
            math::Matrix::CreateScalingMatrix(Scale / 1024.f) *
            math::Matrix::CreateRotationZ(rotZ) *
            math::Matrix::CreateRotationY(rotY) *
            math::Matrix::CreateRotationX(rotX);
    }
};
#pragma pack(pop)
} // namespace input
} // namespace parser