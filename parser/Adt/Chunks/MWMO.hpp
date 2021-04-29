#include "Adt/AdtChunk.hpp"
#include "utility/BinaryStream.hpp"

#include <string>
#include <vector>

namespace parser
{
namespace input
{
class MWMO : AdtChunk
{
public:
    std::vector<std::string> WmoNames;

    MWMO(size_t position, utility::BinaryStream* reader);
};
} // namespace input
} // namespace parser