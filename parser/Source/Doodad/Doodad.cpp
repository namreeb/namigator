#include "MpqManager.hpp"
#include "Doodad/Doodad.hpp"

#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/BinaryStream.hpp"
#include "utility/Include/Exception.hpp"

#include <string>
#include <memory>

namespace parser
{
namespace
{
std::string GetRealModelPath(const std::string &path)
{
    if (path.substr(path.length() - 3, 3) == ".m2" || path.substr(path.length() - 3, 3) == ".M2")
        return path;

    return std::string(path.substr(0, path.rfind('.')) + ".m2");
}
}

Doodad::Doodad(const std::string &path) : Name(GetRealModelPath(path))
{
    std::unique_ptr<utility::BinaryStream> reader(MpqManager::OpenFile(Name));

    if (reader->Read<unsigned int>() != Magic)
        THROW("Invalid doodad file");

    const unsigned int version = reader->Read<unsigned int>();

    switch (version)
    {
        case 256:   // Classic
        case 260:   // TBC
            reader->SetPosition(0xEC);
            break;

        case 264:   // WOTLK
            reader->SetPosition(0xD8);
            break;

        default:
            THROW("Unsupported doodad format");
    }

    const int indexCount = reader->Read<int>();
    const int indicesPosition = reader->Read<int>();
    const int vertexCount = reader->Read<int>();
    const int verticesPosition = reader->Read<int>();

    if (!indexCount || !vertexCount)
        return;

    Vertices.resize(vertexCount);

    // read bounding vertices
    reader->SetPosition(verticesPosition);
    reader->ReadBytes(&Vertices[0], vertexCount * sizeof(utility::Vertex));

    // read bounding indices
    Indices.reserve(indexCount);

    reader->SetPosition(indicesPosition);
    for (auto i = 0; i < indexCount; ++i)
        Indices.push_back(reader->Read<unsigned short>());
}

#ifdef _DEBUG
unsigned int Doodad::MemoryUsage() const
{
    unsigned int ret = sizeof(Doodad);

    ret += Vertices.size() * sizeof(Vertices[0]);
    ret += Indices.size() * sizeof(Indices[0]);

    return ret;
}
#endif
}