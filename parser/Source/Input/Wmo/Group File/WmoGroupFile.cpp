#include <iostream>

#include "Input/WMO/Group File/WmoGroupFile.hpp"

namespace parser_input
{
    WmoGroupFile::WmoGroupFile(const std::string &path) : WowFile(path)
    {
        std::string num = path.substr(path.rfind('_') + 1, 3);
        
        char *cPath = new char[path.length()+1], *p;

        memcpy(cPath, path.c_str(), path.length() + 1);

        p = strrchr(cPath, '.');
        *p = '\0';
        p = strrchr(cPath, '_');
        *p++ = '\0';

        Index = atoi(p);

        delete[] cPath;

        // MOGP

        long mogpOffset = GetChunkLocation("MOGP", 0xC);

        if (mogpOffset < 0)
            throw "No MOGP chunk";

        // since all we want are flags + bounding box, we needn't bother creating an MOGP chunk class
        Reader->SetPosition(mogpOffset + 16);
        Flags = Reader->Read<unsigned int>();

        Reader->ReadBytes((void *)&Min, 3*sizeof(float));
        Reader->ReadBytes((void *)&Max, 3*sizeof(float));

        // MOPY

        long mopyOffset = GetChunkLocation("MOPY", 0x58);

        if (mopyOffset < 0)
            throw "No MOPY chunk";

        MaterialsChunk = new MOPY(mopyOffset, Reader);

        // MOVI

        long moviOffset = GetChunkLocation("MOVI", (int)mopyOffset + 4 + (int)MaterialsChunk->Size);

        if (moviOffset < 0)
            throw "No MOVI chunk";

        IndicesChunk = new MOVI(moviOffset, Reader);

        // MOVT

        long movtOffset = GetChunkLocation("MOVT", (int)moviOffset + 4 + (int)IndicesChunk->Size);

        if (movtOffset < 0)
            throw "No MOVT chunk";

        VerticesChunk = new MOVT(movtOffset, Reader);

        // MLIQ

        long mliqOffset = GetChunkLocation("MLIQ", 0x58);

        // not guarunteed to be present
        LiquidChunk = (mliqOffset >= 0 ? new MLIQ(mliqOffset, Reader) : nullptr);
    }

    WmoGroupFile::~WmoGroupFile()
    {
        delete MaterialsChunk;
        delete IndicesChunk;
        delete VerticesChunk;
        delete LiquidChunk;
    }
}