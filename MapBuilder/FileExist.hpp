#include <string>
#include <filesystem>

namespace file_exist {

bool bvh_files_exist(const std::string& outputPath);

bool map_files_exist(const std::string& outputPath, const std::string& mapName);

} // namespace file_exist

namespace files {

void create_bvh_output_directory(const std::filesystem::path& outputPath);

void create_nav_output_directory(const std::filesystem::path& outputPath);

void create_nav_output_directory_for_map(const std::filesystem::path& outputPath, const std::string& mapName);

} // namespace files
