#include <string>

#include "Output/MpqManager.hpp"
#include "BinaryStream.hpp"

#pragma once

namespace parser_input
{
    class MpqFileReader
    {
        public:
            utility::BinaryStream *BaseStream;

            MpqFileReader(const std::string &mpqFilePath)
            {
                BaseStream = parser::MpqManager::OpenFile(mpqFilePath);
            }

            ~MpqFileReader()
            {
                delete BaseStream;
            }
    };
}