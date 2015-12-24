#include <memory>
#include "BinaryStream.hpp"
#include "Input/ADT/Chunks/MH2O.hpp"

namespace parser_input
{
    MH2O::MH2O(long position, utility::BinaryStream *reader) : AdtChunk(position, reader)
    {
        long offsetFrom = Position + 8;

        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 16; ++x)
            {
                Blocks[y][x] = new MH2OBlock;

                reader->ReadStruct(&Blocks[y][x]->Header);

                if (Blocks[y][x]->Header.LayerCount <= 0)
                {
                    delete Blocks[y][x];
                    Blocks[y][x] = nullptr;
                    continue;
                }

                // jump to (and read) the data struct
                reader->SetPosition(offsetFrom + Blocks[y][x]->Header.DataOffset);
                reader->ReadStruct(&Blocks[y][x]->Data);

                // height data

                // ocean (type 2) has no rendermask, so mark everything as rendered
                if (Blocks[y][x]->Data.Type == 2)
                {
                    for (int i = 0; i < 8; ++i)
                        Blocks[y][x]->RenderMask[i] = 0xFF;

                    for (int heightY = Blocks[y][x]->Data.YOffset; heightY < Blocks[y][x]->Data.YOffset + Blocks[y][x]->Data.Height; ++heightY)
                        for (int heightX = Blocks[y][x]->Data.XOffset; heightX < Blocks[y][x]->Data.XOffset + Blocks[y][x]->Data.Width; ++heightX)
                            Blocks[y][x]->Heights[heightY][heightX] = Blocks[y][x]->Data.HeightLevels[0];
                }
                else
                {
                    // render mask data

                    // jump to (and read) the render mask
                    reader->SetPosition(offsetFrom + Blocks[y][x]->Header.RenderOffset);

                    bool maskEmpty = true;

                    reader->ReadBytes((void *)&Blocks[y][x]->RenderMask, 8);
                    for (int i = 0; i < 8; ++i)
                        if (Blocks[y][x]->RenderMask[i] > 0)
                        {
                            maskEmpty = false;
                            break;
                        }

                    if ((maskEmpty || (Blocks[y][x]->Data.Width == 8 && Blocks[y][x]->Data.Height == 8)) && Blocks[y][x]->Data.MaskOffset)
                    {
                        // try using this other mask (INSTEAD when chunk is 8x8)
                        reader->SetPosition(offsetFrom + Blocks[y][x]->Data.MaskOffset);

                        int maskLength = ((Blocks[y][x]->Data.Height * Blocks[y][x]->Data.Width) % 8 ?
                                            (1 + (Blocks[y][x]->Data.Height * Blocks[y][x]->Data.Width) / 8) :
                                            ((Blocks[y][x]->Data.Height * Blocks[y][x]->Data.Width) / 8));

                        unsigned char *mask = new unsigned char[maskLength];
                        reader->ReadBytes(mask, maskLength);

                        for (int i = 0; i < maskLength; ++i)
                            Blocks[y][x]->RenderMask[i + Blocks[y][x]->Data.YOffset] |= mask[i];
                    }

                    // jump to (and read) the heightmap
                    reader->SetPosition(offsetFrom + Blocks[y][x]->Data.HeightmapOffset);

                    // note that we cannot read these values in at once, as though the values
                    // are sequential in the file, they are not always sequential in memory

                    for (int heightY = Blocks[y][x]->Data.YOffset; heightY < Blocks[y][x]->Data.YOffset + Blocks[y][x]->Data.Height; ++heightY)
                        for (int heightX = Blocks[y][x]->Data.XOffset; heightX < Blocks[y][x]->Data.XOffset + Blocks[y][x]->Data.Width; ++heightX)
                            Blocks[y][x]->Heights[heightY][heightX] = reader->Read<float>();
                }

                // this calculates where we need to be for the next header
                reader->SetPosition(offsetFrom + (y * 16 + x + 1) * 12);
            }
    }

    MH2O::~MH2O()
    {
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 16; ++x)
                if (Blocks[y][x])
                    delete Blocks[y][x];
    }
}