#pragma once
#include <cstdint>

extern "C" {

typedef uint8_t MapBuildResultType;

/*
    Build gameobjects as bounded volume hierachy (BVH).

    `data_path` is the `Data` directory containing `MPQ` files of a client.

    `output_path` is the location where generated data will be placed.

    `threads` is the amount of concurrent jobs that will be used to build the data.

    `amount_of_bvhs_built` returns the amount of gameobjects built.

    This MUST be done before building the maps with `mapbuild_build_map`.
 */
MapBuildResultType mapbuild_build_bvh(const char* const data_path,
                                      const char* const output_path,
                                      uint32_t threads,
                                      uint32_t* const amount_of_bvhs_built);

/*
    Builds a single map.

    `data_path` is the `Data` directory containing `MPQ` files of a client.

    `output_path` is the location where generated data will be placed.

    `threads` is the amount of concurrent jobs that will be used to build the data.

    `map_name` is the name of the map folder inside the MPQ. For example `Azeroth` for Eastern Kingdoms.

    `gameobject_csv` is the path to a CSV file containing a list of dynamically loaded objects.
    This feature is unlikely to work.
*/
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