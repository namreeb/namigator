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

void pathfind_free_map(pathfind::Map* map);

PathfindResultType pathfind_load_all_adts(pathfind::Map* map,
                                          int32_t * amount_of_adts_loaded);

PathfindResultType pathfind_load_adt(pathfind::Map* map,
                                        int adt_x,
                                        int adt_y,
                                        float* out_adt_x,
                                        float* out_adt_y);

PathfindResultType pathfind_load_adt_at(pathfind::Map* map,
                                        float x,
                                        float y,
                                        float* out_adt_x,
                                        float* out_adt_y);

PathfindResultType pathfind_get_zone_and_area(pathfind::Map* map,
                                              float x,
                                              float y,
                                              float z,
                                              unsigned int* out_zone,
                                              unsigned int* out_area);

PathfindResultType pathfind_find_path(pathfind::Map* map,
                                      float start_x,
                                      float start_y,
                                      float start_z,
                                      float stop_x,
                                      float stop_y,
                                      float stop_z,
                                      Vertex* buffer,
                                      unsigned int buffer_length,
                                      unsigned int* amount_of_vertices);

PathfindResultType pathfind_find_heights(pathfind::Map* map,
                                         float x,
                                         float y,
                                         float* buffer,
                                         unsigned int buffer_length,
                                         unsigned int* amount_of_heights);
} // extern "C"

