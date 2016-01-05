#include "Input/WMO/WmoDoodad.hpp"
#include "utility/Include/LinearAlgebra.hpp"

#include <vector>

namespace parser
{
namespace input
{
WmoDoodad::WmoDoodad(const WmoParserInfo *parentInfo, const WmoDoodadInfo *wmoDoodadInfo, const std::string &path) : DoodadFile(path)
{
    const float mid = (533.f + (1.f / 3.f)) * 32.f;
    const float xPos = -(parentInfo->BasePosition.Z - mid);
    const float yPos = -(parentInfo->BasePosition.X - mid);
    const float zPos = parentInfo->BasePosition.Y;

    const utility::Vertex parentOrigin(xPos, yPos, zPos);
    const utility::Vertex origin = wmoDoodadInfo->Position;

    const float parRotX = utility::MathHelper::ToRadians(parentInfo->OrientationC);
    const float parRotY = utility::MathHelper::ToRadians(parentInfo->OrientationA);
    const float parRotZ = utility::MathHelper::ToRadians(parentInfo->OrientationB + 180.f);

    const utility::Quaternion q(wmoDoodadInfo->RotX, wmoDoodadInfo->RotY, wmoDoodadInfo->RotZ, wmoDoodadInfo->RotW);

    const utility::Matrix wmoTransformMatrix = utility::Matrix::CreateScalingMatrix(wmoDoodadInfo->Scale) *
                                                utility::Matrix::CreateFromQuaternion(q);

    const utility::Matrix worldTransformMatrix = utility::Matrix::CreateRotationZ(parRotZ) *
                                                    utility::Matrix::CreateRotationY(parRotY) *
                                                    utility::Matrix::CreateRotationX(parRotX);

    for (size_t i = 0; i < Vertices.size(); ++i)
    {
        auto const wmoVertex = utility::Vertex::Transform(Vertices[i], wmoTransformMatrix) + origin;

        Vertices[i] = utility::Vertex::Transform(wmoVertex, worldTransformMatrix) + parentOrigin;

        if (!i)
            MinZ = MaxZ = Vertices[0].Z;
        else
        {
            if (Vertices[i].Z < MinZ)
                MinZ = Vertices[i].Z;
            if (Vertices[i].Z > MaxZ)
                MaxZ = Vertices[i].Z;
        }
    }
}
}
}