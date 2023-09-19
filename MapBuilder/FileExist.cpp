#include "FileExist.hpp"

namespace fs = std::filesystem;

namespace file_exist {

bool bvh_files_exist(const std::string& outputPath) {
    // bvh.idx is created at the very end of building game objects
    return fs::exists(outputPath + "/BVH/bvh.idx");
}

bool map_files_exist(const std::string& outputPath, const std::string& mapName) {
    // mapName.map is created in MeshBuilder::SaveMap which should be after successful creation
    return fs::exists(outputPath + "/" + mapName + ".map");
}

} // namespace file_exist

namespace files {

void create_bvh_output_directory(const std::filesystem::path& outputPath) {
    fs::create_directories(outputPath / "BVH");
}

void create_nav_output_directory(const std::filesystem::path& outputPath) {
    fs::create_directories(outputPath / "Nav");
}

void create_nav_output_directory_for_map(const std::filesystem::path& outputPath, const std::string& mapName) {
    fs::create_directories(outputPath / "Nav" / mapName);
}

} // namespace files
