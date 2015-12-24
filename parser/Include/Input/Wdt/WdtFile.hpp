#include <string>

#include "Input/WowFile.hpp"
#include "Input/Wmo/WmoParserInfo.hpp"
#include "Input/Wmo/Root File/WmoRootFile.hpp"

namespace parser_input
{
    class WdtFile : WowFile
    {
        public:
            bool HasAdt[64][64];
            bool HasTerrain;

            std::unique_ptr<WmoRootFile> Wmo;

            WdtFile(const std::string &path);
    };
}