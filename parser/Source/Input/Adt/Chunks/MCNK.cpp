#include "Input/ADT/Chunks/MCNK.hpp"
#include "Input/ADT/Chunks/Subchunks/MCRD.hpp"
#include "Input/ADT/Chunks/Subchunks/MCRW.hpp"

namespace parser_input
{
    MCNK::MCNK(long position, utility::BinaryStream *reader, bool findHeader) : AdtChunk(position, reader), Height(0.f)
    {
#ifdef DEBUG
        if (Type != AdtChunkType::MCNK)
            THROW("Expected (but did not find) MCNK type");
#endif

        // it is possible for a chunk to be empty
        if (Size == 0)
            return;

        // if this MCNK is expected to have a header block with it
        if (findHeader)
        {
            MCNKInfo information;

            reader->ReadStruct(&information);

            Height = information.Position[2];

            HeightChunk.reset(new MCVT(Position + information.HeightOffset, reader));

            memset(HoleMap, 0, sizeof(bool)*8*8);

            // holes
            if (HasHoles = information.Holes > 0)
            {
                for (int y = 0; y < 4; ++y)
                    for (int x = 0; x < 4; ++x)
                    {
                        if (!(information.Holes & HoleFlags[y][x]))
                            continue;

                        const int curRow = 1 + (y * 2);
                        const int curCol = 1 + (x * 2);

                        HoleMap[curCol - 1][curRow - 1] =
                            HoleMap[curCol][curRow - 1] =
                            HoleMap[curCol - 1][curRow] =
                            HoleMap[curCol][curRow]        = true;
                    }
            }

            /*
                *       +X
                *     +Y    -Y
                *        -X
                *  When you go up, your ingame X goes up, but the ADT file Y goes down.
                *  When you go left, your ingame Y goes up, but the ADT file X goes down.
                *  When you go down, your ingame X goes down, but the ADT file Y goes up.
                *  When you go right, your ingame Y goes down, but the ADT file X goes up.
                */
            const float quadSize = (533.f + (1.f / 3.f)) / 128.f;

            // Information.IndexX and Information.IndexY specify the colum and row (respectively) of the chunk in the tile
            // Information.Position specifies the top left corner of the current chunk (using in-game coords)
            // these x and y correspond to rows and columns (respectively) of the current chunk

            Positions.reserve(VertexCount);

            for (int i = 0; i < VertexCount; ++i)
            {
                // if i % 17 > 8, this is the inner part of a quad
                const int x = (i % 17 > 8 ? (i - 9) % 17 : i % 17);
                const int y = i / 17;
                const float innerOffset = (i % 17 > 8 ? quadSize / 2.f : 0.f);

                Positions.push_back(Vertex(information.Position[0] - (y*quadSize) - innerOffset,
                                           information.Position[1] - (x*quadSize) - innerOffset,
                                           information.Position[2] + HeightChunk->Heights[i]));
            }

            HasWater = true;
        }
    }
}