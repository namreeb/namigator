#include "Input/M2/DoodadFile.hpp"
#include "utility/Include/Exception.hpp"

namespace parser
{
namespace input
{
std::string DoodadFile::GetRealModelPath(const std::string &path)
{
    if (path.substr(path.length() - 3, 3) == ".m2" || path.substr(path.length() - 3, 3) == ".M2")
        return path;

    return std::string(path.substr(0, path.rfind('.')) + ".m2");
}

DoodadFile::DoodadFile(const std::string &path) : WowFile(GetRealModelPath(path))
{
    if (Reader->Read<unsigned int>() != Magic)
        THROW("Invalid doodad file");

    const unsigned int version = Reader->Read<unsigned int>();

    switch (version)
    {
        // Classic
        case 256:
            Reader->SetPosition(0xEC);
            break;

        // TBC
        case 260:
            Reader->SetPosition(0xEC);
            break;

        // WOTLK
        case 264:
            Reader->SetPosition(0xD8);
            break;

        default:
            THROW("Unsupported doodad format");
    }

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

    Reader->ReadBytes(&Vertices[0], vertexCount * sizeof(utility::Vertex));

    // read bounding indices
    Reader->SetPosition(indicesPosition);

    Reader->ReadBytes(&Indices[0], indexCount * sizeof(unsigned short));
}

Doodad::Doodad(const std::string &path, const DoodadInfo &doodadInfo) : DoodadFile(path)
{
    constexpr float mid = utility::MathHelper::AdtSize * 32.f;

    const float xPos = -(doodadInfo.BasePosition.Z - mid);
    const float yPos = -(doodadInfo.BasePosition.X - mid);
    const float zPos = doodadInfo.BasePosition.Y;
    const utility::Vertex origin(xPos, yPos, zPos);

    const float rotX = utility::Convert::ToRadians(doodadInfo.OrientationC);
    const float rotY = utility::Convert::ToRadians(doodadInfo.OrientationA);
    const float rotZ = utility::Convert::ToRadians(doodadInfo.OrientationB + 180.f);

    const utility::Matrix transformMatrix = utility::Matrix::CreateScalingMatrix(doodadInfo.Scale / 1024.f) *
                                            utility::Matrix::CreateRotationZ(rotZ) *
                                            utility::Matrix::CreateRotationY(rotY) *
                                            utility::Matrix::CreateRotationX(rotX);

    for (size_t i = 0; i < Vertices.size(); ++i)
    {
        Vertices[i] = utility::Vertex::Transform(Vertices[i], transformMatrix) + origin;

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
}