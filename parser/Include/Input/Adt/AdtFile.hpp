#pragma once

#include <string>
#include <memory>

#include "Input/ADT/Chunks/MHDR.hpp"
#include "Input/ADT/Chunks/MCNK.hpp"
#include "Input/ADT/Chunks/MH2O.hpp"
#include "Input/ADT/Chunks/MWMO.hpp"
#include "Input/ADT/Chunks/MODF.hpp"
#include "Input/ADT/Chunks/MMDX.hpp"
#include "Input/ADT/Chunks/MDDF.hpp"
#include "Input/WMO/Root File/WmoRootFile.hpp"
#include "Input/M2/DoodadFile.hpp"

#include "LinearAlgebra.hpp"

#define ADT_VERSION    18

namespace parser_input
{
    class AdtFile : public WowFile
    {
        public:
            std::string m_mapName;
            int X;
            int Y;

            unsigned int m_version;

            std::unique_ptr<MCNK> m_chunks[16][16];

            bool m_hasMH2O;
            bool m_hasMCLQ;

            std::unique_ptr<MH2O> m_liquidChunk;

            std::unique_ptr<MODF> m_wmoChunk;
            std::unique_ptr<MDDF> m_doodadChunk;

            std::vector<std::string> m_wmoNames;
            std::vector<std::string> m_doodadNames;

            AdtFile(const std::string &path);
    };
}