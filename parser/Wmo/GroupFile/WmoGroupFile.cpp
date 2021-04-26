#include "Wmo/GroupFile/WmoGroupFile.hpp"

#include "MpqManager.hpp"

#include "utility/Exception.hpp"

#include <memory>

namespace parser
{
namespace input
{
WmoGroupFile::WmoGroupFile(const std::string &path)
{
    std::unique_ptr<utility::BinaryStream> reader(sMpqManager.OpenFile(path));

    // MOGP
    size_t mogpLocation;
    if (!reader->GetChunkLocation("MOGP", 12, mogpLocation))
        THROW("No MOGP chunk");

    reader->rpos(mogpLocation + 8 + 0x08);  // +0x08: flags
    Flags = reader->Read<std::uint32_t>();

    reader->rpos(mogpLocation + 8 + 0x38);  // +0x38: area id
    GroupId = reader->Read<std::uint32_t>();

    // MOPY
    size_t mopyLocation;
    if (!reader->GetChunkLocation("MOPY", 0x58, mopyLocation))
        THROW("No MOPY chunk");

    MaterialsChunk = std::make_unique<MOPY>(mopyLocation, reader.get());

    // MOVI
    size_t moviLocation;
    if (!reader->GetChunkLocation("MOVI", mopyLocation, moviLocation))
        THROW("No MOVI chunk");

    IndicesChunk = std::make_unique<MOVI>(moviLocation, reader.get());

    // MOVT
    size_t movtLocation;
    if (!reader->GetChunkLocation("MOVT", moviLocation, movtLocation))
        THROW("No MOVT chunk");

    VerticesChunk = std::make_unique<MOVT>(movtLocation, reader.get());

    // MLIQ -- not guarunteed to be present
    size_t mliqLocation;
    LiquidChunk.reset(reader->GetChunkLocation("MLIQ", movtLocation, mliqLocation) ? new MLIQ(mliqLocation, reader.get()) : nullptr);
}
}
}