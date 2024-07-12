#include "Map.hpp"
#include "utility/MathHelper.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <optional>

namespace py = pybind11;

namespace
{
py::list python_find_path(const pathfind::Map& map, float start_x,
                          float start_y, float start_z, float stop_x,
                          float stop_y, float stop_z)
{
    py::list result;

    const math::Vertex start {start_x, start_y, start_z};
    const math::Vertex stop {stop_x, stop_y, stop_z};

    std::vector<math::Vertex> path;

    if (map.FindPath(start, stop, path))
        for (auto const& point : path)
            result.append(py::make_tuple(point.X, point.Y, point.Z));

    return result;
}

py::tuple load_adt(pathfind::Map& map, int adt_x, int adt_y)
{
    if (!map.HasADT(adt_x, adt_y))
        throw std::runtime_error("Requested ADT does not exist for map");

    if (!map.LoadADT(adt_x, adt_y))
        throw std::runtime_error("Failed to load requested ADT");

    return py::make_tuple(adt_x, adt_y);
}

py::tuple load_adt_at(pathfind::Map& map, float x, float y)
{
    int adt_x, adt_y;

    math::Convert::WorldToAdt({x, y, 0.f}, adt_x, adt_y);

    if (!map.HasADT(adt_x, adt_y))
        throw std::runtime_error("Requested ADT does not exist for map");

    if (!map.LoadADT(adt_x, adt_y))
        throw std::runtime_error("Failed to load requested ADT");

    return py::make_tuple(adt_x, adt_y);
}

void unload_adt(pathfind::Map& map, int adt_x, int adt_y) {
    map.UnloadADT(adt_x, adt_y);
}

bool adt_loaded(pathfind::Map& map, int adt_x, int adt_y) {
    return map.IsADTLoaded(adt_x, adt_y);
}

bool has_adts(pathfind::Map& map) {
    return map.HasADTs();
}

py::list python_query_heights(const pathfind::Map& map, float x, float y)
{
    py::list result;

    std::vector<float> height_values;

    if (map.FindHeights(x, y, height_values))
        for (auto const& z : height_values)
            result.append(z);

    return result;
}

std::optional<float> python_query_z(const pathfind::Map& map, float start_x, float start_y, float start_z,
    float stop_x, float stop_y)
{
    float result;
    if (!map.FindHeight({start_x, start_y, start_z}, stop_x, stop_y, result))
        return {};
    return result;
}

bool los(const pathfind::Map& map, float start_x, float start_y, float start_z,
         float stop_x, float stop_y, float stop_z, bool doodads)
{
    return map.LineOfSight(
            {start_x, start_y, start_z},
            {stop_x, stop_y, stop_z},
            doodads);
}

py::object get_zone_and_area(pathfind::Map& map, float x, float y, float z)
{
    math::Vertex p {x, y, z};
    unsigned int zone = -1, area = -1;
    if (!map.ZoneAndArea(p, zone, area))
        return py::none();
    return py::make_tuple(zone, area);
}

py::object find_point_in_between_vectors(pathfind::Map& map, float distance, float x1, float y1, float z1, float x2, float y2, float z2)
{
    const math::Vertex start {x1, y1, z1};
    const math::Vertex end {x2, y2, z2};
    math::Vertex in_between_point {};
    if (!map.FindPointInBetweenVectors(start, end, distance, in_between_point)) {
        return py::none();
    }

    return py::make_tuple(in_between_point.X, in_between_point.Y, in_between_point.Z);
}

py::object find_random_point_around_circle(pathfind::Map& map, float x, float y, float z, float radius) {
    const math::Vertex start {x, y, z};

    math::Vertex random_point {};
    if (!map.FindRandomPointAroundCircle(start, radius, random_point)) {
        return py::none();
    }

    return py::make_tuple(random_point.X, random_point.Y, random_point.Z);
}

} // namespace

PYBIND11_MODULE(pathfind, m)
{
    py::class_<pathfind::Map>(m, "Map")
        .def(py::init<const std::string&, const std::string&>(),
            py::arg("data_path"),
            py::arg("map_name")
        )
        .def("load_all_adts",
            &pathfind::Map::LoadAllADTs,
            R"del(Loads all ADTs for the map in order for pathfinding to be immediately available everywhere on the map.

This may take a while depending on the map size.)del"
        )
        .def("load_adt_at",
            &load_adt_at,
            "Load ADT at specific map coordinate.",
            py::arg("x"),
            py::arg("y")
        )
        .def("load_adt",
            &load_adt,
            "Load specific ADT.",
            py::arg("adt_x"),
            py::arg("adt_y")
        )
        .def(
            "find_path",
           &python_find_path,
           R"del(Attempts to find a path between `start` and `stop`.

Returns a list of points if a path was found, otherwise an empty list.)del",
           py::arg("start_x"),
           py::arg("start_y"),
           py::arg("start_z"),
           py::arg("stop_x"),
           py::arg("stop_y"),
           py::arg("stop_z")
        )
        .def("query_heights",
            &python_query_heights,
            "Finds all Z values for a given `x`, `y` coordinate.",
            py::arg("x"),
            py::arg("y")
        )
        .def("query_z",
            &python_query_z,
            R"del(Returns the `stop_z` value for a given `start_x`, `start_y`, `start_z` and `stop_x`, `stop_y`.

This is the value that would be achieved by walking from start to stop.)del",
            py::arg("start_x"),
            py::arg("start_y"),
            py::arg("start_z"),
            py::arg("stop_x"),
            py::arg("stop_y")
        )
        .def("get_zone_and_area",
            &get_zone_and_area,
            "Returns the zone and area values for a specific coordinate.",
            py::arg("x"),
            py::arg("y"),
            py::arg("z")
        )
        .def("find_point_in_between_vectors",
            &find_point_in_between_vectors,
            "Returns a point in between two vectors given a distance.",
            py::arg("distance"),
            py::arg("x1"),
            py::arg("y1"),
            py::arg("z1"),
            py::arg("x2"),
            py::arg("y2"),
            py::arg("z2")
        )
        .def("find_random_point_around_circle",
            &find_random_point_around_circle,
            "Returns a random point from a circle within or slightly outside of the given radius.",
            py::arg("x"),
            py::arg("y"),
            py::arg("z"),
            py::arg("radius")
        )
        .def("has_adts",
            &has_adts,
            "Checks if the map has any ADT."
        )
        .def("adt_loaded",
            &adt_loaded,
            "Checks if a specific ADT is loaded.",
            py::arg("adt_x"),
            py::arg("adt_y")
        )
        .def("unload_adt",
            &unload_adt,
            "Unloads a specific ADT.",
            py::arg("adt_x"),
            py::arg("adt_y")
        )
        .def("line_of_sight",
            &los,
            R"del(Checks for line of sight from `start` to `stop`.

If `doodads` is `False` doodads will not be considered during calculations.)del",
            py::arg("start_x"),
            py::arg("start_y"),
            py::arg("start_z"),
            py::arg("stop_x"),
            py::arg("stop_y"),
            py::arg("stop_z"),
            py::arg("doodads")
        );
}
