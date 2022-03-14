#include "GameObjectBVHBuilder.hpp"
#include "MeshBuilder.hpp"
#include "Worker.hpp"
#include "parser/MpqManager.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#define PY_SSIZE_T_CLEAN
#ifdef _DEBUG
#    undef _DEBUG
#    include <Python.h>
#    define _DEBUG
#else
#    include <Python.h>
#endif
namespace fs = std::filesystem;

namespace
{
PyObject* BuildBVH(PyObject *self, PyObject *args)
{
    char *data_path_str, *output_path_str;
    unsigned long long workers;

    if (!PyArg_ParseTuple(args, "ssK", &data_path_str, &output_path_str,
                          &workers))
        return nullptr;

    const fs::path data_path {data_path_str};
    const fs::path output_path {output_path_str};

    parser::sMpqManager.Initialize(data_path.string());

    if (!fs::is_directory(output_path))
        fs::create_directory(output_path);

    if (!fs::is_directory(output_path / "BVH"))
        fs::create_directory(output_path / "BVH");

    parser::GameObjectBVHBuilder goBuilder(output_path.string(),
                                           output_path.string(), workers);

    goBuilder.Begin();

    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while (goBuilder.Remaining() > 0);

    return PyLong_FromSize_t(goBuilder.Shutdown());
}

PyObject* BuildMap(PyObject* self, PyObject* args)
{
    char *data_path_str, *output_path_str, *map_name, *go_csv_path_str;
    unsigned long long num_workers;

    if (!PyArg_ParseTuple(args, "sssKs", &data_path_str, &output_path_str, &map_name,
                          &num_workers, &go_csv_path_str))
        return Py_False;

    const fs::path data_path {data_path_str};
    const fs::path output_path {output_path_str};
    const fs::path go_csv_path {go_csv_path_str};

    if (!num_workers)
        return Py_False;

    parser::sMpqManager.Initialize(data_path.string());

    if (!fs::is_directory(output_path))
        fs::create_directory(output_path);

    if (!fs::is_directory(output_path / "BVH"))
        fs::create_directory(output_path / "BVH");

    if (!fs::is_directory(output_path / "Nav"))
        fs::create_directory(output_path / "Nav");

    std::unique_ptr<MeshBuilder> builder;
    std::vector<std::unique_ptr<Worker>> workers;

    try
    {
        builder =
            std::make_unique<MeshBuilder>(output_path.string(), map_name, 0);

        if (!go_csv_path.empty())
            builder->LoadGameObjects(go_csv_path.string());

        for (auto i = 0u; i < num_workers; ++i)
            workers.push_back(
                std::make_unique<Worker>(data_path.string(), builder.get()));
    }
    catch (std::exception const& e)
    {
        std::cerr << "Builder initialization failed: " << e.what() << std::endl;
        return Py_False;
    }

    for (;;)
    {
        bool done = true;
        for (auto const& worker : workers)
            if (!worker->IsFinished())
            {
                done = false;
                break;
            }

        if (done)
            break;

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    builder->SaveMap();

    return Py_True;
}

PyObject* BuildADT(PyObject* self, PyObject* args)
{
    char *data_path_str, *output_path_str, *map_name, *go_csv_path_str;
    int x, y;

    if (!PyArg_ParseTuple(args, "sssiis", &data_path_str, &output_path_str,
                          &map_name, &x, &y, &go_csv_path_str))
        return Py_False;

    const fs::path data_path {data_path_str};
    const fs::path output_path {output_path_str};
    const fs::path go_csv_path {go_csv_path_str};

    if (x < 0 || y < 0)
        return Py_False;

    if (!fs::is_directory(output_path))
        fs::create_directory(output_path);

    if (!fs::is_directory(output_path / "BVH"))
        fs::create_directory(output_path / "BVH");

    if (!fs::is_directory(output_path / "Nav"))
        fs::create_directory(output_path / "Nav");

    std::unique_ptr<MeshBuilder> builder;
    std::vector<std::unique_ptr<Worker>> workers;

    builder =
        std::make_unique<MeshBuilder>(output_path.string(), map_name, 0, x, y);

    if (!go_csv_path.empty())
        builder->LoadGameObjects(go_csv_path.string());

    if (builder->IsGlobalWMO())
        return Py_False;

    workers.push_back(
        std::make_unique<Worker>(data_path.string(), builder.get()));

    for (;;)
    {
        bool done = true;
        for (auto const& worker : workers)
            if (!worker->IsFinished())
            {
                done = false;
                break;
            }

        if (done)
            break;

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return Py_True;
}

static PyMethodDef mapbuild_methods[] = {
    {"build_bvh", BuildBVH, METH_VARARGS, "Builds BVH data"},
    {"build_map", BuildMap, METH_VARARGS, "Builds map"},
    {"build_adt", BuildADT, METH_VARARGS, "Builds ADT"},
    {nullptr}
};

struct PyModuleDef mapbuild_module = {
    PyModuleDef_HEAD_INIT,
    "mapbuild",
    "namigator map building library",
    -1,
    mapbuild_methods
};
} // namespace

PyMODINIT_FUNC PyInit_mapbuild(void)
{
    return PyModule_Create(&mapbuild_module);
}
