#include <string>
#include "Log.hpp"

#define VERSION "MAPS001"

namespace parser
{
    class Parser
    {
        public:
            static utility::Logger Log;

            static void Initialize();
    };
}