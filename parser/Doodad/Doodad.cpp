#include "MpqManager.hpp"
#include "Doodad/Doodad.hpp"

#include "utility/Vector.hpp"
#include "utility/BinaryStream.hpp"
#include "utility/Exception.hpp"

#include <string>
#include <memory>
#include <cstdint>

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

Doodad::Doodad(const std::string &path)
{
    FileName = path.substr(path.rfind('\\') + 1);
    FileName = FileName.substr(0, FileName.rfind('.'));

    std::unique_ptr<utility::BinaryStream> reader(MpqManager::OpenFile(GetRealModelPath(path)));

    if (!reader)
        THROW("Doodad " + path + " not found");

    if (reader->Read<std::uint32_t>() != Magic)
        THROW("Invalid doodad file");

    auto const version = reader->Read<std::uint32_t>();

    switch (version)
    {
        case 256:   // Classic
        case 260:   // TBC
        case 261:   // TBC
        case 262:   // TBC
        case 263:   // TBC
            reader->rpos(0xEC);
            break;

        case 272:   // TBC
        case 264:   // WOTLK
            reader->rpos(0xD8);
            break;

        default:
            THROW("Unsupported doodad format: " + version);
    }

    auto const indexCount = reader->Read<std::uint32_t>();
    auto const indicesPosition = reader->Read<std::uint32_t>();
    auto const vertexCount = reader->Read<std::uint32_t>();
    auto const verticesPosition = reader->Read<std::uint32_t>();

    if (!indexCount || !vertexCount)
        return;

    Vertices.resize(vertexCount);

    // read bounding vertices
    reader->rpos(verticesPosition);
    reader->ReadBytes(&Vertices[0], vertexCount * sizeof(math::Vertex));

    // read bounding indices
    Indices.reserve(indexCount);

    reader->rpos(indicesPosition);
    for (auto i = 0u; i < indexCount; ++i)
        Indices.push_back(reader->Read<std::uint16_t>());
}
}