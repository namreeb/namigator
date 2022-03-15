#include "Map.hpp"
#include "utility/MathHelper.hpp"

#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <exception>

#define PY_SSIZE_T_CLEAN
#ifdef _DEBUG
#    undef _DEBUG
#    include <Python.h>
#    define _DEBUG
#else
#    include <Python.h>
#endif

namespace
{

struct MapObject
{
    PyObject_HEAD
    pathfind::Map *map;
};

int map_object_init(MapObject* map_obj, PyObject* args, PyObject* kwds)
{
    char *data_dir, *map_name;

    if (!PyArg_ParseTuple(args, "ss", &data_dir, &map_name))
        return -1;

    try
    {
        map_obj->map = new pathfind::Map(data_dir, map_name);
    }
    catch (std::exception const &e)
    {
        PyErr_SetString(PyExc_Exception, e.what());
        return -1;
    }

    return 0;
}

void map_object_dealloc(MapObject* self)
{
    if (self->map)
        delete self->map;
    Py_TYPE(self)->tp_free((PyObject*)self);
}

PyObject* find_path(MapObject* map_obj, PyObject *args)
{
    float start_x, start_y, start_z, stop_x, stop_y, stop_z;

    if (!PyArg_ParseTuple(args, "ffffff", &start_x, &start_y, &start_z, &stop_x,
                          &stop_y, &stop_z))
        return nullptr;

    const math::Vertex start {start_x, start_y, start_z};
    const math::Vertex stop {stop_x, stop_y, stop_z};

    std::vector<math::Vertex> path;

    if (!map_obj->map->FindPath(start, stop, path))
        return PyList_New(0);

    auto result = PyList_New(path.size());
    for (auto i = 0u; i < path.size(); ++i)
    {
        auto point_tuple = PyTuple_New(3);
        PyTuple_SetItem(point_tuple, 0, PyFloat_FromDouble(path[i].X));
        PyTuple_SetItem(point_tuple, 1, PyFloat_FromDouble(path[i].Y));
        PyTuple_SetItem(point_tuple, 2, PyFloat_FromDouble(path[i].Z));

        PyList_SetItem(result, i, point_tuple);
    }

    return result;
}

PyObject* load_all_adts(MapObject* map_obj)
{
    auto const result = map_obj->map->LoadAllADTs();
    return PyLong_FromLong(result);
}

PyObject* load_adt_at(MapObject* map_obj, PyObject* args)
{
    float x, y;

    if (!PyArg_ParseTuple(args, "ff", &x, &y))
        return nullptr;

    int adt_x, adt_y;

    math::Convert::WorldToAdt({x, y, 0.f}, adt_x, adt_y);

    if (!map_obj->map->HasADT(adt_x, adt_y))
    {
        PyErr_SetString(PyExc_Exception,
                        "Requested ADT does not exist for map");
        return nullptr;
    }

    if (!map_obj->map->LoadADT(adt_x, adt_y))
    {
        PyErr_SetString(PyExc_Exception, "Failed to load requested ADT");
        return nullptr;
    }

    auto result = PyTuple_New(2);
    PyTuple_SetItem(result, 0, PyLong_FromLong(adt_x));
    PyTuple_SetItem(result, 1, PyLong_FromLong(adt_y));
    return result;
}

PyObject* query_z(MapObject* map_obj, PyObject* args)
{
    float x, y;

    if (!PyArg_ParseTuple(args, "ff", &x, &y))
        return nullptr;

    std::vector<float> height_values;

    if (!map_obj->map->FindHeights(x, y, height_values))
        return PyList_New(0);

    auto result = PyList_New(height_values.size());
    for (auto i = 0u; i < height_values.size(); ++i)
        PyList_SetItem(result, i, PyFloat_FromDouble(height_values[i]));

    return result;
}

PyObject* get_zone_and_area(MapObject* map_obj, PyObject* args)
{
    float x, y, z;

    if (!PyArg_ParseTuple(args, "fff", &x, &y, &z))
        return nullptr;

    math::Vertex p {x, y, z};
    unsigned int zone = -1, area = -1;

    if (!map_obj->map->ZoneAndArea(p, zone, area))
        return Py_None;

    auto result = PyTuple_New(2);
    PyTuple_SetItem(result, 0, PyLong_FromLong(zone));
    PyTuple_SetItem(result, 1, PyLong_FromLong(area));
    return result;
}

PyMethodDef map_methods[] = {
    {"load_all_adts", reinterpret_cast<PyCFunction>(load_all_adts), METH_NOARGS,
     "Load all ADTs"},
    {"load_adt_at", reinterpret_cast<PyCFunction>(load_adt_at), METH_VARARGS,
     "Load ADT at given x, y"},
    {"find_path", reinterpret_cast<PyCFunction>(find_path), METH_VARARGS,
     "Find path from A to B"},
    {"query_z", reinterpret_cast<PyCFunction>(query_z), METH_VARARGS,
     "Query Z values at given x, y"},
    {"get_zone_and_area", reinterpret_cast<PyCFunction>(get_zone_and_area),
     METH_VARARGS, "Get zone and area at given x,y,z"},
    {nullptr}};

PyModuleDef pathfind_module = {
    PyModuleDef_HEAD_INIT,
    "pathfind",
    "namigator pathfind library",
    -1
};
} // namespace

PyMODINIT_FUNC PyInit_pathfind(void)
{
    static PyTypeObject map_type = {PyVarObject_HEAD_INIT(NULL, 0)};
    map_type.tp_name = "pathfind.Map";
    map_type.tp_basicsize = sizeof(MapObject);
    map_type.tp_getattro = PyObject_GenericGetAttr;
    map_type.tp_flags = Py_TPFLAGS_DEFAULT;
    map_type.tp_doc = "Pathfind Map";
    map_type.tp_methods = map_methods;
    map_type.tp_init = (initproc)map_object_init;
    map_type.tp_dealloc = (destructor)map_object_dealloc;
    map_type.tp_new = PyType_GenericNew;

    if (PyType_Ready(&map_type) < 0)
        return nullptr;

    auto module = PyModule_Create(&pathfind_module);
    if (!module)
        return nullptr;

    Py_INCREF(&map_type);
    PyModule_AddObject(module, "Map", (PyObject*)&map_type);
    return module;
}