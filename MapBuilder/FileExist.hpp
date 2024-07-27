#include <string>
#include <filesystem>
#include <vector>

namespace file_exist {

bool bvh_files_exist(const std::string& outputPath);

bool map_files_exist(const std::string& outputPath, const std::string& mapName);

bool wow_exist_at_root(const std::string& rootPath);

} // namespace file_exist

namespace dir_exist {

bool output_dir_exist(const std::string& rootPath);

} // namespace dir_exist

namespace files {

void create_output_directory(const std::string& rootPath);

void create_bvh_output_directory(const std::filesystem::path& outputPath);

void create_nav_output_directory(const std::filesystem::path& outputPath);

void create_nav_output_directory_for_map(const std::filesystem::path& outputPath, const std::string& mapName);

std::vector<std::string> get_directories(const std::filesystem::path& mapsPath);

} // namespace files
