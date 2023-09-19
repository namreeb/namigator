#include "GameObjectBVHBuilder.hpp"
#include "MeshBuilder.hpp"
#include "Worker.hpp"
#include "parser/MpqManager.hpp"
#include "FileExist.hpp"

#include <pybind11/pybind11.h>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
namespace py = pybind11;

int BuildBVH(const std::string& dataPath, const std::string& outputPath,
             size_t workers)
{
    parser::sMpqManager.Initialize(dataPath);

    files::create_bvh_output_directory(outputPath);

    parser::GameObjectBVHBuilder goBuilder(dataPath, outputPath, workers);

    goBuilder.Begin();

    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while (goBuilder.Remaining() > 0);

    return static_cast<int>(goBuilder.Shutdown());
}

bool BuildMap(const std::string& dataPath, const std::string& outputPath,
              const std::string& mapName, size_t threads,
              const std::string& goCSV)
{
    if (!threads)
        return false;

    parser::sMpqManager.Initialize(dataPath);

    files::create_bvh_output_directory(outputPath);
    files::create_nav_output_directory(outputPath);

    std::unique_ptr<MeshBuilder> builder;
    std::vector<std::unique_ptr<Worker>> workers;

    try
    {
        builder = std::make_unique<MeshBuilder>(outputPath, mapName, 0);

        if (!goCSV.empty())
            builder->LoadGameObjects(goCSV);

        for (auto i = 0u; i < threads; ++i)
            workers.push_back(
                std::make_unique<Worker>(dataPath, builder.get()));
    }
    catch (std::exception const& e)
    {
        std::cerr << "Builder initialization failed: " << e.what() << std::endl;
        return false;
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

    return true;
}

bool BuildADT(const std::string& dataPath, const std::string& outputPath,
              const std::string& mapName, int x, int y,
              const std::string& goCSV)
{
    if (x < 0 || y < 0)
        return false;

    files::create_bvh_output_directory(outputPath);
    files::create_nav_output_directory(outputPath);

    std::unique_ptr<MeshBuilder> builder;
    std::vector<std::unique_ptr<Worker>> workers;

    builder = std::make_unique<MeshBuilder>(outputPath, mapName, 0, x, y);

    if (!goCSV.empty())
        builder->LoadGameObjects(goCSV);

    if (builder->IsGlobalWMO())
        return false;

    workers.push_back(std::make_unique<Worker>(dataPath, builder.get()));

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

    return true;
}

bool BVHFilesExist(const std::string& outputPath) {
    return file_exist::bvh_files_exist(outputPath);
}

bool MapFilesExist(const std::string& outputPath, const std::string& mapName) {
    return file_exist::map_files_exist(outputPath, mapName);
}

PYBIND11_MODULE(mapbuild, m)
{
    m.def("build_bvh",
        BuildBVH,
        "Builds all gameobjects. Must be called before `build_map`.",
        py::arg("data_path"),
        py::arg("output_path"),
        py::arg("workers")
    );
    m.def("build_map",
        &BuildMap,
        "Builds a specific map. `build_bvh` must be called before this function.",
        py::arg("data_path"),
        py::arg("output_path"),
        py::arg("map_name"),
        py::arg("threads"),
        py::arg("go_csv")
    );
    m.def("build_adt",
         &BuildADT,
         "Build a specific ADT.",
         py::arg("data_path"),
         py::arg("output_path"),
         py::arg("map_name"),
         py::arg("x"),
         py::arg("y"),
         py::arg("go_csv")
    );
    m.def("map_files_exist",
         &MapFilesExist,
         "Checks if map files exist. If `True` the map will not need to be built.",
         py::arg("output_path"),
         py::arg("map_name")
    );
    m.def("bvh_files_exist",
         &BVHFilesExist,
         "Checks if gameobjects exist. If `True` the gameobjects will not need to be built.",
         py::arg("output_path")
    );
}