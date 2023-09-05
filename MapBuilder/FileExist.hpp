#include <string>

namespace file_exist {

bool bvh_files_exist(const std::string& outputPath);

bool map_files_exist(const std::string& outputPath, const std::string& mapName);

} // namespace file_exist