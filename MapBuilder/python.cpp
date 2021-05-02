#include "GameObjectBVHBuilder.hpp"
#include "MeshBuilder.hpp"
#include "Worker.hpp"
#include "parser/MpqManager.hpp"

#include <boost/python.hpp>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

int BuildBVH(const std::string& dataPath, const std::string& outputPath,
             size_t workers)
{
    parser::sMpqManager.Initialize(dataPath);

    if (!fs::is_directory(outputPath))
        fs::create_directory(outputPath);

    if (!fs::is_directory(outputPath + "/BVH"))
        fs::create_directory(outputPath + "/BVH");

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

    if (!fs::is_directory(outputPath))
        fs::create_directory(outputPath);

    if (!fs::is_directory(outputPath + "/BVH"))
        fs::create_directory(outputPath + "/BVH");

    if (!fs::is_directory(outputPath + "/Nav"))
        fs::create_directory(outputPath + "/Nav");

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

    if (!fs::is_directory(outputPath))
        fs::create_directory(outputPath);

    if (!fs::is_directory(outputPath + "/BVH"))
        fs::create_directory(outputPath + "/BVH");

    if (!fs::is_directory(outputPath + "/Nav"))
        fs::create_directory(outputPath + "/Nav");

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

BOOST_PYTHON_MODULE(mapbuild)
{
    boost::python::def("build_bvh", BuildBVH);
    boost::python::def("build_map", BuildMap);
    boost::python::def("build_adt", BuildADT);
}