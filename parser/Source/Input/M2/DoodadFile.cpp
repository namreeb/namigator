#include "Input/M2/DoodadFile.hpp"

namespace parser_input
{
    std::string DoodadFile::GetRealModelPath(const std::string &path)
    {
        if (path.substr(path.length() - 3, 3) == ".m2" || path.substr(path.length() - 3, 3) == ".M2")
            return path;

        return std::string(path.substr(0, path.rfind('.')) + ".m2");
    }

    DoodadFile::DoodadFile(const std::string &path) : WowFile(GetRealModelPath(path))
    {
        Reader->SetPosition(0xEC);

        const int indexCount = Reader->Read<int>();
        const int indicesPosition = Reader->Read<int>();
        const int vertexCount = Reader->Read<int>();
        const int verticesPosition = Reader->Read<int>();

        if (!indexCount || !vertexCount)
            return;

        Vertices.resize(vertexCount);
        Indices.resize(indexCount);

        // read bounding vertices
        Reader->SetPosition(verticesPosition);

        Reader->ReadBytes(&Vertices[0], vertexCount * sizeof(Vertex));

        // read bounding indices
        Reader->SetPosition(indicesPosition);

        Reader->ReadBytes(&Indices[0], indexCount * sizeof(unsigned short));
    }

    Doodad::Doodad(const std::string &path, DoodadInfo doodadInfo) : DoodadFile(path)
    {
        const float mid = (float)((533.0 + (1.0/3.0)) * 32.0);
        float xPos = -(doodadInfo.BasePosition.Z - mid);
        float yPos = -(doodadInfo.BasePosition.X - mid);
        float zPos = doodadInfo.BasePosition.Y;
        Vertex origin(xPos, yPos, zPos);

        float rotX = MathHelper::ToRadians(doodadInfo.OrientationC);
        float rotY = MathHelper::ToRadians(doodadInfo.OrientationA);
        float rotZ = MathHelper::ToRadians(doodadInfo.OrientationB + 180.f);
        Matrix<float> transformMatrix = Matrix<float>::CreateScalingMatrix(doodadInfo.Scale / 1024.f) *
                                        Matrix<float>::CreateRotationX(rotX) *
                                        Matrix<float>::CreateRotationY(rotY) *
                                        Matrix<float>::CreateRotationZ(rotZ);

        for (unsigned int i = 0; i < Vertices.size(); ++i)
        {
            Vertices[i] = Vertex::Transform(Vertices[i], transformMatrix) + origin;

            if (!i)
                MinZ = MaxZ = Vertices[i].Z;
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