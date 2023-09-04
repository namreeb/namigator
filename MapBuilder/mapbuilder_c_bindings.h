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

/*
    Tests if gameobjects have been built in `output_path`.

    `exists` will be `0` if the gameobject files haven't been built, and `1` if they have.
 */
MapBuildResultType mapbuild_bvh_files_exist(const char* const output_path,
                                            uint8_t* const exists);

/*
    Tests if the map with `map_name` has already been built in `output_path`.

    `exists` will be `0` if the map hasn't been built, and `1` if it has.
 */
MapBuildResultType mapbuild_map_files_exist(const char* const output_path,
                                            const char* const map_name,
                                            uint8_t* const exists);

}