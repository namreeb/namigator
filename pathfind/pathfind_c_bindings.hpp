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

pathfind::Map* pathfind_new_map(const char* const data_path,
                                const char* const map_name,
                                PathfindResultTypePtr result);

void pathfind_free_map(pathfind::Map* const map);

PathfindResultType pathfind_load_all_adts(pathfind::Map* const map,
                                          int32_t* const amount_of_adts_loaded);

PathfindResultType pathfind_load_adt(pathfind::Map* const map, int adt_x, int adt_y,
                                     float* const out_adt_x, float* const out_adt_y);

PathfindResultType pathfind_load_adt_at(pathfind::Map* const map, float x, float y,
                                        float* const out_adt_x, float* const out_adt_y);

PathfindResultType pathfind_get_zone_and_area(pathfind::Map* const map, float x,
                                              float y, float z,
                                              unsigned int* const out_zone,
                                              unsigned int* const out_area);

PathfindResultType pathfind_find_path(pathfind::Map* const map, float start_x,
                                      float start_y, float start_z,
                                      float stop_x, float stop_y, float stop_z,
                                      Vertex* const buffer,
                                      unsigned int buffer_length,
                                      unsigned int* const amount_of_vertices);

PathfindResultType pathfind_find_heights(pathfind::Map* const map, float x, float y,
                                         float* const buffer,
                                         unsigned int buffer_length,
                                         unsigned int* const amount_of_heights);

PathfindResultType pathfind_find_height(pathfind::Map* const map, float start_x,
                                        float start_y, float start_z,
                                        float stop_x, float stop_y,
                                        float* const stop_z);
} // extern "C"

