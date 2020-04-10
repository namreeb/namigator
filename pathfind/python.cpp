#include "Map.hpp"

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
}

BOOST_PYTHON_MODULE(pathfind)
{
    py::class_<pathfind::Map, boost::noncopyable>
        ("Map", py::init<const std::string &, const std::string &>())
        .def("load_all_adts", &pathfind::Map::LoadAllADTs)
        .def("find_path", &python_find_path);
}