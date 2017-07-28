#include "utility/Include/BinaryStream.hpp"
#include "Adt/Chunks/MH2O.hpp"

#include <memory>
#include <cassert>
#include <vector>
#include <iostream>
#include <thread>
#include <cstdint>

namespace parser
{
namespace input
{
MH2O::MH2O(size_t position, utility::BinaryStream *reader) : AdtChunk(position, reader)
{
    const size_t offsetFrom = Position + 8;

    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x)
        {
            reader->rpos(offsetFrom + sizeof(MH2OHeader)*(y * 16 + x));
                
            MH2OHeader header;
            reader->ReadBytes(&header, sizeof(header));

            if (header.LayerCount <= 0)
                continue;

            for (int layer = 0; layer < header.LayerCount; ++layer)
            {
                reader->rpos(offsetFrom + header.InstancesOffset + layer*sizeof(LiquidInstance));

                LiquidInstance instance;
                reader->ReadBytes(&instance, sizeof(instance));

                assert(!!instance.Width && !!instance.Height);

                auto newLayer = new LiquidLayer();

                newLayer->X = x;
                newLayer->Y = y;

                memset(newLayer->Render, 0, sizeof(newLayer->Render));
                memset(newLayer->Heights, 0, sizeof(newLayer->Heights));

                std::vector<std::uint8_t> exists(instance.OffsetVertexData - instance.OffsetExistsBitmap);
                std::vector<float> heightMap((instance.Width + 1)*(instance.Height + 1));

                if (!exists.size())
                {
                    exists.resize(instance.Width * instance.Height);
                    memset(&exists[0], 0xFF, exists.size());

                    assert((instance.MaxHeight - instance.MinHeight) < 0.001f);
                    assert(!std::isnan(instance.MaxHeight));

                    for (size_t i = 0; i < heightMap.size(); ++i)
                        heightMap[i] = instance.MaxHeight;
                }
                else
                {
                    if (header.AttributesOffset && instance.OffsetExistsBitmap)
                    {
                        reader->rpos(offsetFrom + instance.OffsetExistsBitmap);
                        reader->ReadBytes(&exists[0], exists.size());
                    }
                    else
                        memset(&exists[0], 0xFF, exists.size());

                    // ocean?  no height map!
                    if (instance.Type == 2)
                    {
                        for (size_t i = 0; i < heightMap.size(); ++i)
                            heightMap[i] = instance.MaxHeight;
                    }
                    else
                    {
                        reader->rpos(offsetFrom + instance.OffsetVertexData);
                        reader->ReadBytes(&heightMap[0], sizeof(float)*heightMap.size());
                    }
                }

                int currentByte = 0;
                int currentBit = 0;
                for (int squareY = 0; squareY < instance.Height; ++squareY)
                    for (int squareX = 0; squareX < instance.Width; ++squareX)
                    {
                        if (currentBit == 8)
                        {
                            currentByte++;
                            currentBit = 0;
                        }

                        if (!((exists[currentByte] >> currentBit++) & 0x01))
                            continue;

                        newLayer->Render[squareY + instance.YOffset][squareX + instance.XOffset] = true;

                        newLayer->Heights[squareY + instance.YOffset + 0][squareX + instance.XOffset + 0] = heightMap[(squareY + 0)*(instance.Width + 1) + squareX + 0];
                        newLayer->Heights[squareY + instance.YOffset + 0][squareX + instance.XOffset + 1] = heightMap[(squareY + 0)*(instance.Width + 1) + squareX + 1];
                        newLayer->Heights[squareY + instance.YOffset + 1][squareX + instance.XOffset + 0] = heightMap[(squareY + 1)*(instance.Width + 1) + squareX + 0];
                        newLayer->Heights[squareY + instance.YOffset + 1][squareX + instance.XOffset + 1] = heightMap[(squareY + 1)*(instance.Width + 1) + squareX + 1];
                    }

                Layers.push_back(std::unique_ptr<LiquidLayer>(newLayer));
            }
        }
}
}
}