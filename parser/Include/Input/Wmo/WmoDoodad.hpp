#include "Input/M2/DoodadFile.hpp"
#include "Input/Wmo/WmoParserInfo.hpp"

namespace parser_input
{
    struct WmoDoodadInfo
    {
        unsigned int NameIndex;
        Vertex Position;
        float RotW;
        float RotX;
        float RotY;
        float RotZ;
        float Scale;
        unsigned int Color;
    };

    struct WmoDoodadIndexedInfo
    {
        int Index;
        WmoDoodadInfo DoodadInfo;
    };

    class WmoDoodad : public DoodadFile
    {
        public:
            float MinZ;
            float MaxZ;

            WmoDoodad(const WmoParserInfo *parentInfo, const WmoDoodadInfo *wmoDoodadInfo, const std::string &path);
    };
}