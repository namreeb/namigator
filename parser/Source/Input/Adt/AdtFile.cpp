#include <string>

#include "Input/ADT/AdtFile.hpp"

namespace parser_input
{
    AdtFile::AdtFile(const std::string &path) : WowFile(path), m_hasMCLQ(false)
    {
        // parse path "World\Maps\zone\zone_X_Y...."
        char *cPath = new char[path.length()+1], *zone, *x, *y, *p;
        memcpy(cPath, path.c_str(), path.length()+1);

        zone = strrchr(cPath, '\\') + 1;

        x = strchr(zone, '_');
        *x++ = '\0';
        y = strchr(x, '_');
        *y++ = '\0';

        for (p = y; *p >= '0' && *p <= '9'; ++p);
        *p = '\0';

        m_mapName = std::string(zone);
        X = atoi(x);
        Y = atoi(y);

        delete[] cPath;

        if (Reader->Read<unsigned int>() != AdtChunkType::MVER)
            THROW("MVER does not begin ADT file");

        Reader->Slide(4);

        if ((m_version = Reader->Read<unsigned int>()) != ADT_VERSION)
            THROW("ADT version is incorrect");

        MHDR header(GetChunkLocation("MHDR", Reader->GetPosition()), Reader);

        if (m_hasMH2O = !!header.Offsets.Mh2oOffset)
            m_liquidChunk = std::unique_ptr<MH2O>(new MH2O(header.Offsets.Mh2oOffset + 0x14, Reader));

        long currMcnk = GetChunkLocation("MCNK");

        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 16; ++x)
            {
                m_chunks[y][x].reset(new MCNK(currMcnk, Reader));
                currMcnk += 8 + m_chunks[y][x]->Size;

                if (m_chunks[y][x]->HasWater)
                    m_hasMCLQ = true;
            }

        // MMDX
        const long mmdxLocation = GetChunkLocation("MMDX", header.Offsets.MmdxOffset + 0x14);

        if (mmdxLocation >= 0)
        {
            MMDX doodadNamesChunk(mmdxLocation, Reader);
            m_doodadNames = std::move(doodadNamesChunk.DoodadNames);
        }

        // MMID: XXX - other people read in the MMID chunk here.  do we actually need it?

        // MWMO
        const long mwmoLocation = GetChunkLocation("MWMO", header.Offsets.MwmoOffset + 0x14);

        if (mwmoLocation >= 0)
        {
            MWMO wmoNamesChunk(mwmoLocation, Reader);
            m_wmoNames = std::move(wmoNamesChunk.WmoNames);
        }

        // MWID: XXX - other people read in the MWID chunk here.  do we actually need it?

        // MDDF
        const long mddfLocation = GetChunkLocation("MDDF", header.Offsets.MddfOffset + 0x14);
        m_doodadChunk = mddfLocation < 0 ? nullptr : std::unique_ptr<MDDF>(new MDDF(mddfLocation, Reader));

        // MODF
        const long modfLocation = GetChunkLocation("MODF", header.Offsets.ModfOffset + 0x14);
        m_wmoChunk = modfLocation < 0 ? nullptr : std::unique_ptr<MODF>(new MODF(modfLocation, Reader));
    }
}