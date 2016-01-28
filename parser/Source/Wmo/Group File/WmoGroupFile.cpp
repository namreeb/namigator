#include "MpqManager.hpp"
#include "Wmo/Group File/WmoGroupFile.hpp"

#include "utility/Include/Exception.hpp"

#include <memory>

namespace parser
{
namespace input
{
WmoGroupFile::WmoGroupFile(const std::string &path)
{
    std::unique_ptr<utility::BinaryStream> reader(MpqManager::OpenFile(path));

    // MOGP
    size_t mogpLocation;
    if (!reader->GetChunkLocation("MOGP", 12, mogpLocation))
        THROW("No MOGP chunk");

    // since all we want are flags + bounding box, we needn't bother creating an MOGP chunk class
    reader->SetPosition(mogpLocation + 16);
    Flags = reader->Read<unsigned int>();

    // MOPY
    size_t mopyLocation;
    if (!reader->GetChunkLocation("MOPY", 0x58, mopyLocation))
        THROW("No MOPY chunk");

    MaterialsChunk.reset(new MOPY(mopyLocation, reader.get()));

    // MOVI
    size_t moviLocation;
    if (!reader->GetChunkLocation("MOVI", mopyLocation, moviLocation))
        THROW("No MOVI chunk");

    IndicesChunk.reset(new MOVI(moviLocation, reader.get()));

    // MOVT
    size_t movtLocation;
    if (!reader->GetChunkLocation("MOVT", moviLocation, movtLocation))
        THROW("No MOVT chunk");

    VerticesChunk.reset(new MOVT(movtLocation, reader.get()));

    // MLIQ -- not guarunteed to be present
    size_t mliqLocation;
    LiquidChunk.reset(reader->GetChunkLocation("MLIQ", movtLocation, mliqLocation) ? new MLIQ(mliqLocation, reader.get()) : nullptr);
}
}
}