#include "Wmo/GroupFile/WmoGroupFile.hpp"

#include "MpqManager.hpp"
#include "utility/Exception.hpp"

#include <memory>

namespace
{
void ReadWmoGroupFile(parser::input::WmoGroupFile *groupFile,
    unsigned int version,
    utility::BinaryStream *reader)
{
    // MOGP
    size_t mogpLocation;
    if (!reader->GetChunkLocation("MOGP", reader->rpos(), mogpLocation))
        THROW(Result::NO_MOGP_CHUNK);

    reader->rpos(mogpLocation + 4);
    auto const mogpSize = reader->Read<std::uint32_t>();

    reader->rpos(reader->rpos() + 0x08); // +0x08: flags
    groupFile->Flags = reader->Read<std::uint32_t>();

    // MOPY
    size_t mopyLocation;
    if (!reader->GetChunkLocation("MOPY", mogpLocation+0x4C, mopyLocation))
        THROW(Result::NO_MOPY_CHUNK);

    groupFile->MaterialsChunk =
        std::make_unique<parser::input::MOPY>(version, mopyLocation, reader);

    // for alpha data, this chunk is called "MOIN" but has the same format as
    // newer versions

    // MOVI
    size_t moviLocation;
    if (!reader->GetChunkLocation(version == 14 ? "MOIN" : "MOVI", mopyLocation,
                                  moviLocation))
        THROW(Result::NO_MOVI_CHUNK);

    groupFile->IndicesChunk =
        std::make_unique<parser::input::MOVI>(moviLocation, reader);

    // MOVT.  we use the MOPY location as the hint because in the alpha data
    // this chunk comes sooner
    size_t movtLocation;
    if (!reader->GetChunkLocation("MOVT", mopyLocation, movtLocation))
        THROW(Result::NO_MOVT_CHUNK);

    groupFile->VerticesChunk =
        std::make_unique<parser::input::MOVT>(movtLocation, reader);

    // MLIQ -- not guaranteed to be present
    size_t mliqLocation;
    groupFile->LiquidChunk.reset(
        reader->GetChunkLocation("MLIQ", movtLocation, mliqLocation) ?
            new parser::input::MLIQ(mliqLocation, reader, version) :
            nullptr);

    // we jump the reader ahead here.  in the case of the alpha data, this will
    // leave the cursor in the right place.  otherwise, the cursor is about to
    // be destroyed, so it shouldn't matter
    reader->rpos(mogpLocation + 8 + mogpSize);
}
}

namespace parser
{
namespace input
{
WmoGroupFile::WmoGroupFile(unsigned int version, const std::string& path)
{
    auto reader = sMpqManager.OpenFile(path);
    ReadWmoGroupFile(this, version, reader.get());

}
WmoGroupFile::WmoGroupFile(unsigned int version, utility::BinaryStream *reader)
{
    ReadWmoGroupFile(this, version, reader);
}
} // namespace input
} // namespace parser