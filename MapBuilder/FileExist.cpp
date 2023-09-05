#include "FileExist.hpp"

#include <filesystem>

namespace file_exist {

bool bvh_files_exist(const std::string& outputPath) {
    // bvh.idx is created at the very end of building game objects
    return std::filesystem::exists(outputPath + "/BVH/bvh.idx");
}

bool map_files_exist(const std::string& outputPath, const std::string& mapName) {
    // mapName.map is created in MeshBuilder::SaveMap which should be after successful creation
    return std::filesystem::exists(outputPath + "/" + mapName + ".map");
}

} // namespace file_exist
