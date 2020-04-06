#include "Map.hpp"

#include <boost/python.hpp>

#include <string>
#include <memory>
#include <vector>
#include <thread>
#include <experimental/filesystem>

namespace py = boost::python;

BOOST_PYTHON_MODULE(pathfind)
{
    py::class_<pathfind::Map, boost::noncopyable>
        ("Map", py::init<const std::string &, const std::string &>())
        .def("load_adt", &pathfind::Map::LoadADT)
        .def("unload_adt", &pathfind::Map::UnloadADT)
        //.def("find_height", &pathfind::Map::FindHeight)
        .def("find_heights", &pathfind::Map::FindHeights)
        .def("find_path", &pathfind::Map::FindPath);
}