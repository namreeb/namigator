#include "Input/WMO/WmoDoodad.hpp"

namespace parser_input
{
    WmoDoodad::WmoDoodad(const WmoParserInfo *parentInfo, const WmoDoodadInfo *wmoDoodadInfo, const std::string &path) : DoodadFile(path)
    {
        const float mid = (533.f + (1.f / 3.f)) * 32.f;
        const float xPos = -(parentInfo->BasePosition.Z - mid);
        const float yPos = -(parentInfo->BasePosition.X - mid);
        const float zPos = parentInfo->BasePosition.Y;

        const Vertex parentOrigin(xPos, yPos, zPos);
        const Vertex origin = wmoDoodadInfo->Position;

        const float parRotX = MathHelper::ToRadians(parentInfo->OrientationC);
        const float parRotY = MathHelper::ToRadians(parentInfo->OrientationA);
        const float parRotZ = MathHelper::ToRadians(parentInfo->OrientationB - 90.f);

        //Quaternion q = Quaternion(-wmoDoodadInfo->RotY, wmoDoodadInfo->RotZ, -wmoDoodadInfo->RotX, wmoDoodadInfo->RotW);
        // according to MaiN, from Greyman:
        Quaternion q = Quaternion(wmoDoodadInfo->RotX, wmoDoodadInfo->RotY, wmoDoodadInfo->RotZ, wmoDoodadInfo->RotZ);

        Matrix transformMatrix = Matrix::CreateScalingMatrix(wmoDoodadInfo->Scale) *
                                        Matrix::CreateRotationY(PI) *
                                        Matrix::CreateFromQuaternion(q) *
                                        Matrix::CreateTranslationMatrix(origin) *
                                        Matrix::CreateRotationX(parRotX) *
                                        Matrix::CreateRotationY(parRotY) *
                                        Matrix::CreateRotationZ(parRotZ) *
                                        Matrix::CreateTranslationMatrix(parentOrigin);

        for (unsigned int i = 0; i < Vertices.size(); ++i)
        {
            Vertices[i] = Vertex::Transform(Vertices[i], transformMatrix);

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