#pragma once

namespace parser_input
{
    struct MOHD
    {
        unsigned int TexturesCount;
        int WMOGroupFilesCount;
        unsigned int PortalsCount;
        unsigned int LightsCount;
        unsigned int DoodadNamesCount;
        unsigned int DoodadDefsCount;
        unsigned int DoodadSetsCount;

        unsigned char ColR;
        unsigned char ColG;
        unsigned char ColB;
        unsigned char ColX;

        unsigned int WMOId;

        float BoundingBox1[3];
        float BoundingBox2[3];

        unsigned int Unknown;
    };
}