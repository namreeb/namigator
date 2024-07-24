#include "pathfind_c_bindings.hpp"

#include "utility/Exception.hpp"
#include "utility/MathHelper.hpp"

extern "C" {

pathfind::Map* pathfind_new_map(const char* const data_path, const char* const map_name,
                              PathfindResultTypePtr result) {
    try
    {
        *result = static_cast<PathfindResultType>(Result::SUCCESS);
        return new pathfind::Map(data_path, map_name);
    }
    catch (utility::exception& e)
    {
        *result = static_cast<PathfindResultType>(e.ResultCode());
        return nullptr;
    }
    catch (...) {
        *result = static_cast<PathfindResultType>(Result::UNKNOWN_EXCEPTION);
        return nullptr;
    }
}

void pathfind_free_map(pathfind::Map* const map) {
    delete map;
}

PathfindResultType pathfind_load_all_adts(pathfind::Map* const map, int32_t* const amount_of_adts_loaded) {
    try {
        *amount_of_adts_loaded = map->LoadAllADTs();
        return static_cast<PathfindResultType>(Result::SUCCESS);
    }
    catch (utility::exception& e) {
        return static_cast<PathfindResultType>(e.ResultCode());
    }
    catch (...) {
        return static_cast<PathfindResultType>(Result::UNKNOWN_EXCEPTION);
    }
}

PathfindResultType pathfind_load_adt(pathfind::Map* const map, int adt_x, int adt_y, float* const out_adt_x, float* const out_adt_y) {
    try {
        if (!map->HasADT(adt_x, adt_y)) {
            return static_cast<PathfindResultType>(Result::MAP_DOES_NOT_HAVE_ADT);
        }

        if (!map->LoadADT(adt_x, adt_y)) {
            return static_cast<PathfindResultType>(Result::FAILED_TO_LOAD_ADT);
        }

        *out_adt_x = static_cast<float>(adt_x);
        *out_adt_y = static_cast<float>(adt_y);

        return static_cast<PathfindResultType>(Result::SUCCESS);
    }
    catch (utility::exception& e) {
        return static_cast<PathfindResultType>(e.ResultCode());
    }
    catch (...) {
        return static_cast<PathfindResultType>(Result::UNKNOWN_EXCEPTION);
    }
}

PathfindResultType pathfind_load_adt_at(pathfind::Map* const map, float x, float y, float* const out_adt_x, float* const out_adt_y) {
    try {
        int adt_y = 0;
        int adt_x = 0;

        math::Convert::WorldToAdt({x, y, 0.f}, adt_x, adt_y);

        if (!map->HasADT(adt_x, adt_y)) {
            return static_cast<PathfindResultType>(Result::MAP_DOES_NOT_HAVE_ADT);
        }

        if (!map->LoadADT(adt_x, adt_y)) {
            return static_cast<PathfindResultType>(Result::FAILED_TO_LOAD_ADT);
        }

        *out_adt_x = static_cast<float>(adt_x);
        *out_adt_y = static_cast<float>(adt_y);

        return static_cast<PathfindResultType>(Result::SUCCESS);
    }
    catch (utility::exception& e) {
        return static_cast<PathfindResultType>(e.ResultCode());
    }
    catch (...) {
        return static_cast<PathfindResultType>(Result::UNKNOWN_EXCEPTION);
    }
}

PathfindResultType pathfind_unload_adt(pathfind::Map* const map, int x, int y) {
    try {
        map->UnloadADT(x, y);
    }
    catch (utility::exception& e) {
        return static_cast<PathfindResultType>(e.ResultCode());
    }
    catch (...) {
        return static_cast<PathfindResultType>(Result::UNKNOWN_EXCEPTION);
    }

    return static_cast<PathfindResultType>(Result::SUCCESS);
}

PathfindResultType pathfind_is_adt_loaded(pathfind::Map* const map, int x, int y, uint8_t* const loaded) {
    try {
        if (map->IsADTLoaded(x, y)) {
            *loaded = 1;
        } else{
            *loaded = 0;
        }
    }
    catch (utility::exception& e) {
        return static_cast<PathfindResultType>(e.ResultCode());
    }
    catch (...) {
        return static_cast<PathfindResultType>(Result::UNKNOWN_EXCEPTION);
    }

    return static_cast<PathfindResultType>(Result::SUCCESS);
}

PathfindResultType pathfind_has_adts(pathfind::Map* const map, bool* has_adts) {
    try {
        *has_adts = map->HasADTs();
        return static_cast<PathfindResultType>(Result::SUCCESS);
    }
    catch (utility::exception& e) {
        return static_cast<PathfindResultType>(e.ResultCode());
    }
    catch (...) {
        return static_cast<PathfindResultType>(Result::UNKNOWN_EXCEPTION);
    }
}

PathfindResultType pathfind_get_zone_and_area(pathfind::Map* const map,
                       float x,
                       float y,
                       float z,
                       unsigned int* const out_zone,
                       unsigned int* const out_area)
{
    unsigned int zone = -1;
    unsigned int area = -1;

    math::Vertex p {x, y, z};

    try {
        const auto success = map->ZoneAndArea(p, zone, area);
        *out_zone = zone;
        *out_area = area;

        if (success) {
            return static_cast<PathfindResultType>(Result::SUCCESS);
        } else {
            return static_cast<PathfindResultType>(Result::UNKNOWN_ZONE_AND_AREA);
        }
    }
    catch (utility::exception& e) {
        return static_cast<PathfindResultType>(e.ResultCode());
    }
    catch (...) {
        return static_cast<PathfindResultType>(Result::UNKNOWN_EXCEPTION);
    }
}

PathfindResultType pathfind_find_point_in_between_vectors(pathfind::Map* const map,
                                                          float distance,
                                                          float x1,
                                                          float y1,
                                                          float z1,
                                                          float x2,
                                                          float y2,
                                                          float z2,
                                                          Vertex* out_vertex) {
    try {
        const math::Vertex start {x1, y1, z1};
        const math::Vertex end {x2, y2, z2};
        math::Vertex in_between_point {};
        if (!map->FindPointInBetweenVectors(start, end, distance, in_between_point)) {
            return static_cast<PathfindResultType>(Result::FAILED_TO_FIND_POINT_BETWEEN_VECTORS);
        }

        out_vertex->x = in_between_point.X;
        out_vertex->y = in_between_point.Y;
        out_vertex->z = in_between_point.Z;

        return static_cast<PathfindResultType>(Result::SUCCESS);
    }
    catch (utility::exception& e) {
        return static_cast<PathfindResultType>(e.ResultCode());
    }
    catch (...) {
        return static_cast<PathfindResultType>(Result::UNKNOWN_EXCEPTION);
    }
}

PathfindResultType pathfind_find_path(pathfind::Map* const map,
               float start_x,
               float start_y,
               float start_z,
               float stop_x,
               float stop_y,
               float stop_z,
               Vertex* const buffer,
               unsigned int buffer_length,
               unsigned int* const amount_of_vertices)
{
    const math::Vertex start {start_x, start_y, start_z};
    const math::Vertex stop {stop_x, stop_y, stop_z};

    std::vector<math::Vertex> path;

    try {
        if (map->FindPath(start, stop, path)) {
            if (path.size() > buffer_length) {
                *amount_of_vertices = static_cast<unsigned int>(path.size());
                return static_cast<PathfindResultType>(Result::BUFFER_TOO_SMALL);
            }

            auto vbuf = static_cast<Vertex*>(static_cast<void*>(buffer));

            for (int i = 0; i < path.size(); ++i) {
                const auto point = path[i];
                const auto v = Vertex { point.X, point.Y, point.Z};
                vbuf[i] = v;
            }

            *amount_of_vertices = static_cast<unsigned int>(path.size());

            return static_cast<PathfindResultType>(Result::SUCCESS);
        } else {
            return static_cast<PathfindResultType>(Result::UNKNOWN_PATH);
        }
    }
    catch (utility::exception& e) {
        return static_cast<PathfindResultType>(e.ResultCode());
    }
    catch (...) {
        return static_cast<PathfindResultType>(Result::UNKNOWN_EXCEPTION);
    }
}

PathfindResultType pathfind_find_heights(pathfind::Map* const map,
                  float x,
                  float y,
                  float* const buffer,
                  unsigned int buffer_length,
                  unsigned int* const amount_of_heights)
{
    std::vector<float> height_values;

    try {
        if (map->FindHeights(x, y, height_values)) {
            if (buffer_length < height_values.size()) {
                return static_cast<PathfindResultType>(Result::BUFFER_TOO_SMALL);
            }

            for (auto i = 0; i < height_values.size(); ++i) {
                buffer[i] = height_values[i];
            }

            *amount_of_heights = static_cast<unsigned int>(height_values.size());

            return static_cast<PathfindResultType>(Result::SUCCESS);
        }

        return static_cast<PathfindResultType>(Result::UNKNOWN_HEIGHT);
    }
    catch (utility::exception& e) {
        return static_cast<PathfindResultType>(e.ResultCode());
    }
    catch (...) {
        return static_cast<PathfindResultType>(Result::UNKNOWN_EXCEPTION);
    }
}

PathfindResultType pathfind_find_height(pathfind::Map* const map, float start_x,
    float start_y, float start_z,
    float stop_x, float stop_y,
    float* const stop_z)
{
    try
    {
        math::Vertex start {start_x, start_y, start_z};
        float result;
        if (map->FindHeight(start, stop_x, stop_y, result))
        {
            *stop_z = result;
            return static_cast<PathfindResultType>(Result::SUCCESS);
        }

        return static_cast<PathfindResultType>(Result::UNKNOWN_HEIGHT);
    }
    catch (utility::exception& e)
    {
        return static_cast<PathfindResultType>(e.ResultCode());
    }
    catch (...)
    {
        return static_cast<PathfindResultType>(Result::UNKNOWN_EXCEPTION);
    }
}

PathfindResultType pathfind_line_of_sight(pathfind::Map* map,
                                          float start_x, float start_y, float start_z,
                                          float stop_x, float stop_y, float stop_z,
                                          uint8_t* const line_of_sight, uint8_t doodads) {
    try
    {
        if (map->LineOfSight({start_x, start_y, start_z}, {stop_x, stop_y, stop_z}, doodads)) {
            *line_of_sight = 1;
        } else {
            *line_of_sight = 0;
        }

        return static_cast<PathfindResultType>(Result::SUCCESS);
    }
    catch (utility::exception& e)
    {
        return static_cast<PathfindResultType>(e.ResultCode());
    }
    catch (...)
    {
        return static_cast<PathfindResultType>(Result::UNKNOWN_EXCEPTION);
    }
}

PathfindResultType pathfind_find_random_point_around_circle(pathfind::Map* const map,
                                                            float x,
                                                            float y,
                                                            float z,
                                                            float radius,
                                                            float* const random_x,
                                                            float* const random_y,
                                                            float* const random_z) {

    try
    {
        const math::Vertex start {x, y, z};
        math::Vertex random_point {};

        if (!map->FindRandomPointAroundCircle(start, radius, random_point)) {
            return static_cast<PathfindResultType>(Result::UNABLE_TO_FIND_RANDOM_POINT_IN_CIRCLE);
        }

        *random_x = random_point.X;
        *random_y = random_point.Y;
        *random_z = random_point.Z;

        return static_cast<PathfindResultType>(Result::SUCCESS);
    }
    catch (utility::exception& e)
    {
        return static_cast<PathfindResultType>(e.ResultCode());
    }
    catch (...)
    {
        return static_cast<PathfindResultType>(Result::UNKNOWN_EXCEPTION);
    }
}

} // extern "C"
