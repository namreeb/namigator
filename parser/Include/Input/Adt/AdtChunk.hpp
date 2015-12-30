#pragma once

#include "BinaryStream.hpp"

using namespace utility;

namespace parser_input
{
    class AdtChunkType
    {
        public:
            static const unsigned int MHDR = 'MHDR';
            static const unsigned int MVER = 'MVER';
            static const unsigned int MCIN = 'MCIN';
            static const unsigned int MTEX = 'MTEX';
            static const unsigned int MMDX = 'MMDX';
            static const unsigned int MMID = 'MMID';
            static const unsigned int MWMO = 'MWMO';
            static const unsigned int MWID = 'MWID';
            static const unsigned int MDDF = 'MDDF';
            static const unsigned int MODF = 'MODF';
            static const unsigned int MH2O = 'MH2O';
            static const unsigned int MCNK = 'MCNK';
            static const unsigned int MCLQ = 'MCLQ';
            static const unsigned int MFBO = 'MFBO';
            static const unsigned int MCCV = 'MCCV';
            static const unsigned int MCVT = 'MCVT';
            static const unsigned int MCNR = 'MCNR';
    };

    class WaterChunkType
    {
        public:
            static const unsigned int MH2O = 'MH2O';
            static const unsigned int MCLQ = 'MCLQ';
    };

    class AdtChunk
    {
        public:
            unsigned long Position;
            unsigned int Size;
            unsigned int Type;

            AdtChunk(long position, utility::BinaryStream *reader);
    };
}