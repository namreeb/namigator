#pragma once

#include <string>
#include "BinaryStream.hpp"

#define FILE_DUMP_BUFFER_SIZE    4096

namespace parser_input
{
    class WowFile
    {
        protected:
            long GetAnyChunkLocation(long startLoc);

        public:
            utility::BinaryStream *Reader;
            WowFile(const std::string &path);
            ~WowFile();

            long GetChunkLocation(const std::string &chunkName, int startLoc = 0);
    };
};