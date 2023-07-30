#pragma once
#include <cstdint>

extern "C" {

typedef uint8_t MapBuildResultType;

MapBuildResultType mapbuild_build_bvh(const char* const data_path,
                                      const char* const output_path,
                                      uint32_t threads,
                                      uint32_t* const amount_of_bvhs_built);

MapBuildResultType mapbuild_build_map(const char* const data_path,
                                      const char* const output_path,
                                      const char* const map_name,
                                      const char* const gameobject_csv,
                                      uint32_t threads);

}
