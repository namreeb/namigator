#include "utility/String.hpp"

#include <algorithm>
#include <iterator>
#include <string>

namespace utility
{
std::string lower(const std::string& in)
{
    std::string result;
    std::transform(in.begin(), in.end(), std::back_inserter(result), ::tolower);
    return result;
}
} // namespace utility