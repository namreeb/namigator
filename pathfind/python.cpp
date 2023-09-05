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
        .def(py::init<const std::string&, const std::string&>())
        .def("load_all_adts", &pathfind::Map::LoadAllADTs)
        .def("load_adt_at", &load_adt_at)
        .def("load_adt", &load_adt)
        .def("find_path", &python_find_path)
        .def("query_heights", &python_query_heights)
        .def("query_z", &python_query_z)
        .def("get_zone_and_area", &get_zone_and_area)
        .def("find_random_point_around_circle", &find_random_point_around_circle)
        .def("line_of_sight", &los);
}
