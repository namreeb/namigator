#include "Map.hpp"
#include "utility/MathHelper.hpp"

#include <boost/python.hpp>

#include <string>
#include <memory>
#include <vector>
#include <thread>
#include <experimental/filesystem>

namespace py = boost::python;

namespace
{
boost::python::list python_find_path(const pathfind::Map &map,
    float start_x, float start_y, float start_z,
    float stop_x, float stop_y, float stop_z)
{
    boost::python::list result;

    const math::Vertex start { start_x, start_y, start_z };
    const math::Vertex stop { stop_x, stop_y, stop_z };

    std::vector<math::Vertex> path;

    if (map.FindPath(start, stop, path))
        for (auto const &point : path)
            result.append(boost::python::make_tuple(point.X, point.Y, point.Z));

    return result;
}

boost::python::tuple load_adt_at(pathfind::Map &map, float x, float y)
{
    int adt_x, adt_y;

    math::Convert::WorldToAdt({ x,y,0.f }, adt_x, adt_y);

    if (!map.HasADT(adt_x, adt_y))
        throw std::runtime_error("Requested ADT does not exist for map");

    if (!map.LoadADT(adt_x, adt_y))
        throw std::runtime_error("Failed to load requested ADT");

    return boost::python::make_tuple(adt_x, adt_y);
}

boost::python::list python_query_heights(const pathfind::Map &map, float x,
    float y)
{
    boost::python::list result;

    std::vector<float> height_values;

    if (map.FindHeights(x, y, height_values))
        for (auto const &z : height_values)
            result.append(z);

    return result;
}
}

BOOST_PYTHON_MODULE(pathfind)
{
    py::class_<pathfind::Map, boost::noncopyable>
        ("Map", py::init<const std::string &, const std::string &>())
        .def("load_all_adts", &pathfind::Map::LoadAllADTs)
        .def("load_adt_at", &load_adt_at)
        .def("find_path", &python_find_path)
        .def("query_z", &python_query_heights);
}