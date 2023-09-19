#include "mapbuilder_c_bindings.h"

#include "FileExist.hpp"
#include "GameObjectBVHBuilder.hpp"
#include "MeshBuilder.hpp"
#include "Worker.hpp"
#include "parser/MpqManager.hpp"
#include "utility/Exception.hpp"

extern "C" {

MapBuildResultType mapbuild_build_bvh(const char* const data_path,
                             const char* const output_path,
                             uint32_t threads,
                             uint32_t* const amount_of_bvhs_built)
{
    std::string outputPath = output_path;

    try {
        parser::sMpqManager.Initialize(data_path);

        files::create_bvh_output_directory(outputPath);

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
    catch (...) {
        return static_cast<MapBuildResultType>(Result::UNKNOWN_EXCEPTION);
    }
}

MapBuildResultType mapbuild_build_map(const char* const data_path,
                             const char* const output_path,
                             const char* const map_name,
                             const char* const gameobject_csv,
                             uint32_t threads)
{
    std::filesystem::path outputPath = output_path;

    if (!threads) {
        // Same behavior as internally in GameObjectBVHBuilder
        threads = 1;
    }

    try {
        parser::sMpqManager.Initialize(data_path);

        files::create_bvh_output_directory(outputPath);

        files::create_nav_output_directory(outputPath);
    }
    catch (utility::exception& e) {
        return static_cast<MapBuildResultType >(e.ResultCode());
    }
    catch (...) {
        return static_cast<MapBuildResultType>(Result::UNKNOWN_EXCEPTION);
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
    catch (...) {
        return static_cast<MapBuildResultType>(Result::UNKNOWN_EXCEPTION);
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
    catch (...) {
        return static_cast<MapBuildResultType>(Result::UNKNOWN_EXCEPTION);
    }

    builder->SaveMap();

    return static_cast<MapBuildResultType>(Result::SUCCESS);
}

MapBuildResultType mapbuild_bvh_files_exist(const char* const output_path,
                                            uint8_t* const exists) {
    std::string outputPath = output_path;

    try {
        if (file_exist::bvh_files_exist(outputPath)) {
            *exists = 1;
        } else {
            *exists = 0;
        }
    }
    catch (...) {
        return static_cast<MapBuildResultType>(Result::UNKNOWN_EXCEPTION);
    }

    return static_cast<MapBuildResultType>(Result::SUCCESS);
}

MapBuildResultType mapbuild_map_files_exist(const char* const output_path,
                                            const char* const map_name,
                                            uint8_t* const exists) {

    std::string outputPath = output_path;
    std::string mapName = map_name;

    try {
        if (file_exist::map_files_exist(outputPath, mapName)) {
            *exists = 1;
        } else {
            *exists = 0;
        }
    }
    catch (...) {
        return static_cast<MapBuildResultType>(Result::UNKNOWN_EXCEPTION);
    }

    return static_cast<MapBuildResultType>(Result::SUCCESS);
}

}
