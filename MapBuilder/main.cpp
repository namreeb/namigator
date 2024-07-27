#include "GameObjectBVHBuilder.hpp"
#include "MeshBuilder.hpp"
#include "Worker.hpp"
#include "parser/Adt/Adt.hpp"
#include "parser/MpqManager.hpp"
#include "parser/Wmo/WmoInstance.hpp"
#include "utility/String.hpp"
#include "FileExist.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>

#define STATUS_INTERVAL_SECONDS 10

namespace
{

void DisplayUsage(std::ostream& o)
{
    o << "Usage:\n";
    o << "  -h/--help                      -- Display help message\n";
    o << "  -d/--data <data directory>     -- Path to data directory from "
         "which to draw input geometry\n";
    o << "  -m/--map <map name>            -- Which map to produce data for\n";
    o << "  -b/--bvh                       -- Build line of sight (BVH) data "
         "for all models eligible for spawning by the server\n";
    o << "  -g/--gocsv <go csv file>       -- Path to CSV file containing game "
         "object data to include in static mesh output\n";
    o << "  -o/--output <output directory> -- Path to root output directory\n";
    o << "  -t/--threads <thread count>    -- How many worker threads to use\n";
    o << "  -l/--logLevel <log level>      -- Log level (0 = none, 1 = "
         "progress, 2 = warning, 3 = error)\n";
    o << "  -alpha/--alpha                 -- Automatically extract everything available for alpha client.\n";
#ifdef _DEBUG
    o << "  -x/--adtX <x>                  -- X coordinate of individual ADT "
         "to process\n";
    o << "  -y/--adtY <y>                  -- Y coordinate of individual ADT "
         "to process\n";
#endif
    o.flush();
}

int ExtractBVH(const std::string& goCSVPath, const std::string& dataPath,
               const std::string& outputPath, int threads)
{
    auto lastStatus = static_cast<time_t>(0);
    files::create_bvh_output_directory(outputPath);

    if (!goCSVPath.empty())
    {
        std::cerr << "ERROR: Specifying gameobject data for BVH "
                     "generation is meaningless"
                  << std::endl;
        DisplayUsage(std::cerr);
        return EXIT_FAILURE;
    }

    parser::GameObjectBVHBuilder goBuilder(dataPath, outputPath, threads);

    auto const startSize = goBuilder.Remaining();

    goBuilder.Begin();

    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        auto const now = time(nullptr);

        auto const finished = goBuilder.Remaining() == 0;
        auto const update_status =
            finished || now - lastStatus >= STATUS_INTERVAL_SECONDS;

        // periodically output current status
        if (update_status)
        {
            std::stringstream str;

            auto const completed = startSize - goBuilder.Remaining();

            auto const percentComplete =
                100.f * static_cast<float>(completed) / startSize;
            str << "% Complete: " << std::setprecision(4) << percentComplete
                << " (" << std::dec << completed << " / " << startSize << ")\n";
            std::cout << str.str();
            std::cout.flush();

            lastStatus = now;
        }

        if (finished)
            break;
    } while (true);

    goBuilder.Shutdown();

    std::stringstream fin;
    fin << "Finished BVH generation";
    std::cout << fin.str() << std::endl;

    return EXIT_SUCCESS;
}

int ExtractMap(const std::string& map, const std::string& dataPath, 
               const std::string& outputPath,
               const std::string& goCSVPath, int threads,
               int adtX, int adtY, int logLevel)
{
    auto lastStatus = static_cast<time_t>(0);
    std::unique_ptr<MeshBuilder> builder;
    std::vector<std::unique_ptr<Worker>> workers;

    try
    {
        if (adtX >= 0 && adtY >= 0)
        {
            builder = std::make_unique<MeshBuilder>(outputPath, map, logLevel,
                                                    adtX, adtY);

            if (!goCSVPath.empty())
                builder->LoadGameObjects(goCSVPath);

            if (builder->IsGlobalWMO())
            {
                std::cerr << "ERROR: Specified map has no ADTs" << std::endl;
                DisplayUsage(std::cerr);
                return EXIT_FAILURE;
            }

            std::cout << "Building " << map << " (" << adtX << ", " << adtY
                      << ")..." << std::endl;

            workers.push_back(
                std::make_unique<Worker>(dataPath, builder.get()));
        }
        // either both, or neither should be specified
        else if (adtX >= 0 || adtY >= 0)
        {
            std::cerr << "ERROR: Must specify ADT X and Y" << std::endl;
            DisplayUsage(std::cerr);
            return EXIT_FAILURE;
        }
        else
        {
            builder = std::make_unique<MeshBuilder>(outputPath, map, logLevel);

            if (!goCSVPath.empty())
                builder->LoadGameObjects(goCSVPath);

            for (auto i = 0; i < threads; ++i)
                workers.push_back(
                    std::make_unique<Worker>(dataPath, builder.get()));
        }
    }
    catch (std::exception const& e)
    {
        std::cerr << "Builder initialization failed: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    auto const start = time(nullptr);

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

        auto const now = time(nullptr);

        // periodically output current status
        if (now - lastStatus >= STATUS_INTERVAL_SECONDS)
        {
            std::stringstream str;

            str << "% Complete: " << builder->PercentComplete() << "\n";
            std::cout << str.str();

            lastStatus = now;
        }
    }

    builder->SaveMap();
    auto const runTime = time(nullptr) - start;

    std::cout << "Finished " << map << " (" << builder->CompletedTiles()
              << " tiles) in " << runTime << " seconds." << std::endl;

    return EXIT_SUCCESS;
}

int ExtractAlphaData(std::string& dataPath, std::string& outputPath,
                     const std::string& goCSVPath, int threads, int logLevel)
{
    if (dataPath.empty())
        dataPath = "./Data";

    if (outputPath.empty())
        outputPath = "./Output";

    const std::filesystem::path root(dataPath);
    const std::string rootStr = root.parent_path().string();
    if (!file_exist::wow_exist_at_root(rootStr))
    {
        std::cerr
            << "ERROR: MapBuilder must be placed inside the game root folder"
            << std::endl;
        return EXIT_FAILURE;
    }

    const std::filesystem::path rootOutputPath(outputPath);
    const std::string rootOutputPathStr = rootOutputPath.string();
    if (!dir_exist::output_dir_exist(rootOutputPathStr))
        files::create_output_directory(rootOutputPathStr);

    if (ExtractBVH(goCSVPath, dataPath, outputPath, threads) == EXIT_FAILURE)
        return EXIT_FAILURE;

    files::create_nav_output_directory(outputPath);

    // nav mesh generation requires that the MPQ manager be initialized for
    // the main thread, whereas BVH generation does not.
    parser::sMpqManager.Initialize(dataPath);

    std::string map;
    std::vector<std::string> availableMaps = files::get_directories(dataPath + "/World/Maps/");
    for (auto& element : availableMaps)
    {
        try
        {
            std::filesystem::path path(element);
            map = path.filename().string();
            std::cout << "Building navs for map: " << map << "\n";
            std::cout.flush();
            ExtractMap(path.filename().string(), dataPath, outputPath,
                       goCSVPath, threads, -1, -1, logLevel);
        }
        catch (std::exception const& e)
        {
            std::cerr << "Map: " << map << ", extraction failed : " << e.what()
                      << std::endl;
        }
    }

    return EXIT_SUCCESS;
}

} // namespace

int main(int argc, char* argv[])
{
    std::string dataPath, map, outputPath, goCSVPath;
    int adtX = -1, adtY = -1, threads = 1, logLevel = 1;
    bool bvh = false;
    bool alpha = true;

    try
    {
        for (auto i = 1; i < argc; ++i)
        {
            const std::string arg = utility::lower(argv[i]);
            const bool lastArgument = i == argc - 1;

            if (arg == "-h" || arg == "--help")
            {
                // when this is requested, don't do anything else
                DisplayUsage(std::cout);
                return EXIT_SUCCESS;
            }
            else if (arg == "-alpha" || arg == "--alpha")
            {
                alpha = true;
                continue;
            }
            else if (arg == "-b" || arg == "--bvh")
            {
                bvh = true;
                continue;
            }

            if (lastArgument)
                throw std::invalid_argument("Missing argument to parameter " +
                                            arg);

            // below here we know that there is another argument
            if (arg == "-d" || arg == "--data")
                dataPath = argv[++i];
            else if (arg == "-m" || arg == "--map")
                map = argv[++i];
            else if (arg == "-g" || arg == "--gocsv")
                goCSVPath = argv[++i];
            else if (arg == "-o" || arg == "--output")
                outputPath = argv[++i];
            else if (arg == "-t" || arg == "--threads")
                threads = std::stoi(argv[++i]);
            else if (arg == "-l" || arg == "--loglevel")
                logLevel = std::stoi(argv[++i]);
#ifdef _DEBUG
            else if (arg == "-x" || arg == "--adtX")
                adtX = std::stoi(argv[++i]);
            else if (arg == "-y" || arg == "--adtY")
                adtY = std::stoi(argv[++i]);
#endif
            else
                throw std::invalid_argument("Unrecognized argument " + arg);
        }
    }
    catch (std::invalid_argument const& e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
        DisplayUsage(std::cerr);
        return EXIT_FAILURE;
    }

    // Alpha: Extract BVH and everything inside World/Maps.
    if (alpha) 
    {
        return ExtractAlphaData(dataPath, outputPath, goCSVPath, threads, logLevel);
    }

    if (outputPath.empty())
    {
        std::cerr << "ERROR: Must specify output path" << std::endl;
        DisplayUsage(std::cerr);
        return EXIT_FAILURE;
    }

    if (!bvh && map.empty())
    {
        std::cerr << "ERROR: Must specify either a map to generate (--map) or "
                     "the global generation of BVH data (--bvh)"
                  << std::endl;
        DisplayUsage(std::cerr);
        return EXIT_FAILURE;
    }

    if (adtX >= MeshSettings::Adts || adtY >= MeshSettings::Adts)
    {
        std::cerr << "ERROR: Invalid ADT (" << adtX << ", " << adtY << ")"
                  << std::endl;
        DisplayUsage(std::cerr);
        return EXIT_FAILURE;
    }

    if (bvh)
        return ExtractBVH(goCSVPath, dataPath, outputPath, threads);

    files::create_nav_output_directory(outputPath);
    // nav mesh generation requires that the MPQ manager be initialized for
    // the main thread, whereas BVH generation does not.
    parser::sMpqManager.Initialize(dataPath);

    return ExtractMap(map, dataPath, outputPath, goCSVPath, threads, adtX, adtY, logLevel);
}