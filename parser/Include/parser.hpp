#include <string>
#include "Log.hpp"

#define VERSION "NAMNAV03"

namespace parser
{
    class Parser
    {
        public:
            static utility::Logger Log;

            static void Initialize();
    };
}