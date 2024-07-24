#pragma once
#include "Map.hpp"
#include "Common.hpp"

extern "C" {

typedef struct {
    float x;
    float y;
    float z;
} Vertex;

typedef uint8_t PathfindResultType;
typedef uint8_t* PathfindResultTypePtr;

/*
    Creates a new Map for `map_name` using data from the `data_path`.

    This pointer MUST be freed using `pathfind_free_map`, otherwise it will leak.
 */
pathfind::Map* pathfind_new_map(const char* const data_path,
                                const char* const map_name,
                                PathfindResultTypePtr result);

/*
    Cleans up a map created by `pathfind_new_map`.

    This function will delete the object but will not change the pointer.
*/
void pathfind_free_map(pathfind::Map* const map);

/*
    Loads all ADTs on a map.
*/
PathfindResultType pathfind_load_all_adts(pathfind::Map* const map,
                                          int32_t* const amount_of_adts_loaded);

/*
    Loads specific ADT.
*/
PathfindResultType pathfind_load_adt(pathfind::Map* const map, int adt_x, int adt_y,
                                     float* const out_adt_x, float* const out_adt_y);

/*
    Loads specific ADT at in game coordinate (x, y) and returns the ADT x and y.
*/
PathfindResultType pathfind_load_adt_at(pathfind::Map* const map, float x, float y,
                                        float* const out_adt_x, float* const out_adt_y);

/*
    Unloads specific ADT.
*/
PathfindResultType pathfind_unload_adt(pathfind::Map* const map, int x, int y);

/*
    Checks if a specific ADT is loaded.

    `loaded` is set to `1` if the ADT is loaded, and `0` otherwise.
*/
PathfindResultType pathfind_is_adt_loaded(pathfind::Map* const map, int x, int y, uint8_t* const loaded);


/*
    Returns `true` if the map has any ADTs.
*/
PathfindResultType pathfind_has_adts(pathfind::Map* const map, bool* has_adts);

/*
    Returns the zone and area of a particular x, y, z.
*/
PathfindResultType pathfind_get_zone_and_area(pathfind::Map* const map, float x,
                                              float y, float z,
                                              unsigned int* const out_zone,
                                              unsigned int* const out_area);

/*
    Finds a point between the two vectors with a given distance.
*/
PathfindResultType pathfind_find_point_in_between_vectors(pathfind::Map* const map,
                                                          float distance,
                                                          float x1,
                                                          float y1,
                                                          float z1,
                                                          float x2,
                                                          float y2,
                                                          float z2,
                                                          Vertex* out_vertex);

/*
    Calculates a path from `start_x`, `start_y`, and `start_z` to
    `stop_x`, `stop_y`, and `stop_z`.
*/
PathfindResultType pathfind_find_path(pathfind::Map* const map, float start_x,
                                      float start_y, float start_z,
                                      float stop_x, float stop_y, float stop_z,
                                      Vertex* const buffer,
                                      unsigned int buffer_length,
                                      unsigned int* const amount_of_vertices);

/*
    Slices the map at `x`, `y` and returns all possible `z` values.
*/
PathfindResultType pathfind_find_heights(pathfind::Map* const map, float x, float y,
                                         float* const buffer,
                                         unsigned int buffer_length,
                                         unsigned int* const amount_of_heights);

/*
    Returns the `stop_z` for the path between `start_x`, `start_y`, `start_z`
    and `stop_x`, `stop_y`.
    This is the value that would be achieved by walking from start to stop.
*/
PathfindResultType pathfind_find_height(pathfind::Map* const map, float start_x,
                                        float start_y, float start_z,
                                        float stop_x, float stop_y,
                                        float* const stop_z);

/*
    Calculates whether there is line of sight between `start_x`, `start_y`, `start_z`
    and `stop_x`, `stop_y`, `stop_z`.

    If `doodads` is not `0` doodads will be included in the calculations.
*/
PathfindResultType pathfind_line_of_sight(pathfind::Map* const map,
                                          float start_x, float start_y, float start_z,
                                          float stop_x, float stop_y, float stop_z,
                                          uint8_t* const line_of_sight, uint8_t doodads);

/*
    Returns a random point within `radius` of `x`, `y`, and `z`.
*/
PathfindResultType pathfind_find_random_point_around_circle(pathfind::Map* const map,
                                                            float x,
                                                            float y,
                                                            float z,
                                                            float radius,
                                                            float* const random_x,
                                                            float* const random_y,
                                                            float* const random_z);

} // extern "C"

