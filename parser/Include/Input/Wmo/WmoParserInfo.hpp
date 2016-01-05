#pragma once

#include "utility/Include/LinearAlgebra.hpp"

namespace parser
{
namespace input
{
struct WmoParserInfo
{
    unsigned int NameId;
    unsigned int UniqueId;
    utility::Vertex BasePosition;
    float OrientationA;
    float OrientationB;
    float OrientationC;
    utility::Vertex MaxCorner;
    utility::Vertex MinCorner;
    unsigned short Flags;
    unsigned short DoodadSet;
    unsigned short NameSet;
    unsigned short _pad;
};
}
}