#include "mapbuilder_c_bindings.h"

#include "GameObjectBVHBuilder.hpp"
#include "MeshBuilder.hpp"
#include "Worker.hpp"
#include "parser/MpqManager.hpp"
#include "utility/Exception.hpp"

extern "C" {

MapBuildResultType mapbuild_build_bvh(const char* const data_path,
                             const char* const output_path,
                             uint32_t threads,
                             uint32_t* amount_of_bvhs_built)
{
    std::string outputPath = output_path;

    try {
        parser::sMpqManager.Initialize(data_path);

        if (!fs::is_directory(outputPath))
            fs::create_directory(outputPath);

        if (!fs::is_directory(outputPath + "/BVH"))
            fs::create_directory(outputPath + "/BVH");

        parser::GameObjectBVHBuilder goBuilder(data_path, outputPath, threads);

        goBuilder.Begin();

        do
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } while (goBuilder.Remaining() > 0);

        *amount_of_bvhs_built = static_cast<int>(goBuilder.Shutdown());
        return static_cast<MapBuildResultType>(Result::SUCCESS);
    }
    catch (utility::exception& e) {
        return static_cast<MapBuildResultType>(e.ResultCode());
    }
}

MapBuildResultType mapbuild_build_map(const char* const data_path,
                             const char* const output_path,
                             const char* const map_name,
                             const char* const gameobject_csv,
                             uint32_t threads)
{
    std::string outputPath = output_path;

    if (!threads) {
        // Same behavior as internally in GameObjectBVHBuilder
        threads = 1;
    }

    try {
        parser::sMpqManager.Initialize(data_path);

        if (!fs::is_directory(outputPath)) {
            fs::create_directory(outputPath);
        }

        if (!fs::is_directory(outputPath + "/BVH")) {
            fs::create_directory(outputPath + "/BVH");
        }

        if (!fs::is_directory(outputPath + "/Nav")) {
            fs::create_directory(outputPath + "/Nav");
        }
    }
    catch (utility::exception& e) {
        return static_cast<MapBuildResultType >(e.ResultCode());
    }

    std::unique_ptr<MeshBuilder> builder;
    std::vector<std::unique_ptr<Worker>> workers;

    try
    {
        builder = std::make_unique<MeshBuilder>(outputPath, map_name, 0);

        if (strlen(gameobject_csv) != 0) {
            builder->LoadGameObjects(gameobject_csv);
        }

        for (auto i = 0u; i < threads; ++i) {
            workers.push_back(
                std::make_unique<Worker>(data_path, builder.get()));
        }
    }
    catch (utility::exception& e)
    {
        return static_cast<MapBuildResultType >(e.ResultCode());
    }

    try
    {
    for (;;)
    {
        bool done = true;
        for (auto const& worker : workers) {
            if (!worker->IsFinished()) {
                done = false;
                break;
            }
        }

        if (done) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    }
    catch (utility::exception& e) {
        return static_cast<MapBuildResultType>(e.ResultCode());
    }

    builder->SaveMap();

    return static_cast<MapBuildResultType>(Result::SUCCESS);
}


}
