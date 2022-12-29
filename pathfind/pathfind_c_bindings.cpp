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

void pathfind_free_map(pathfind::Map* map) {
    delete map;
}

PathfindResultType pathfind_load_all_adts(pathfind::Map* map, int32_t* amount_of_adts_loaded) {
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

PathfindResultType pathfind_load_adt(pathfind::Map* map, int adt_x, int adt_y, float* out_adt_x, float* out_adt_y) {
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

PathfindResultType pathfind_load_adt_at(pathfind::Map* map, float x, float y, float* out_adt_x, float* out_adt_y) {
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

PathfindResultType pathfind_get_zone_and_area(pathfind::Map* map,
                       float x,
                       float y,
                       float z,
                       unsigned int* out_zone,
                       unsigned int* out_area)
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

PathfindResultType pathfind_find_path(pathfind::Map* map,
               float start_x,
               float start_y,
               float start_z,
               float stop_x,
               float stop_y,
               float stop_z,
               Vertex* buffer,
               unsigned int buffer_length,
               unsigned int* amount_of_vertices)
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

PathfindResultType pathfind_find_heights(pathfind::Map* map,
                  float x,
                  float y,
                  float* buffer,
                  unsigned int buffer_length,
                  unsigned int* amount_of_heights)
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

} // extern "C"
