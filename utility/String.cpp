#include "string.hpp"

#include <string>
#include <algorithm>
#include <iterator>

namespace utility
{
std::string lower(const std::string &in)
{
    std::string result;
    std::transform(in.begin(), in.end(), std::back_inserter(result), ::tolower);
    return result;
}
}