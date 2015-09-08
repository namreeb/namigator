#pragma once

#include "LinearAlgebra.hpp"

using namespace utility;

namespace parser_input
{
    struct WmoParserInfo
    {
        unsigned int NameId;
        unsigned int UniqueId;
        Vertex BasePosition;
        float OrientationA;
        float OrientationB;
        float OrientationC;
        Vertex MaxCorner;
        Vertex MinCorner;
        unsigned short Flags;
        unsigned short DoodadSet;
        unsigned short NameSet;
        unsigned short _pad;
    };
}