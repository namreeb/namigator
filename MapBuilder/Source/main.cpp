#include "MeshBuilder.hpp"
#include "GameObjectBVHBuilder.hpp"
#include "Worker.hpp"
#include "DBC.hpp"

#include "parser/Include/parser.hpp"
#include "parser/Include/Adt/Adt.hpp"
#include "parser/Include/Wmo/WmoInstance.hpp"

#include "utility/Include/Directory.hpp"

#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <map>
#include <cstdint>

#define STATUS_INTERVAL_SECONDS     10

int main(int argc, char *argv[])
{
    std::string dataPath, map, outputPath;
    int adtX, adtY, jobs, logLevel;

    boost::program_options::options_description desc("Allowed options");
    desc.add_options()
        ("data,d", boost::program_options::value<std::string>(&dataPath)->default_value("."),           "data folder")
        ("map,m", boost::program_options::value<std::string>(&map),                                     "generate data for the specified map")
        ("gameobj,g",                                                                                   "generate line of sight (BVH) data for all game objects (WILL NOT replace existing data)")
        ("output,o", boost::program_options::value<std::string>(&outputPath)->default_value(".\\Maps"), "output path")
        ("adtX,x", boost::program_options::value<int>(&adtX),                                           "adt x")
        ("adtY,y", boost::program_options::value<int>(&adtY),                                           "adt y")
        ("jobs,j", boost::program_options::value<int>(&jobs)->default_value(1),                         "build jobs")
        ("logLevel,l", boost::program_options::value<int>(&logLevel)->default_value(3),                 "log level (0 = none, 1 = progress, 2 = warning, 3 = error)")
        ("help,h",                                                                                      "display help message");

    boost::program_options::variables_map vm;

    try
    {
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
        boost::program_options::notify(vm);
    }
    catch (boost::program_options::error const &e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
        std::cerr << desc << std::endl;

        return EXIT_FAILURE;
    }

    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return EXIT_SUCCESS;
    }

    if (vm.count("map") + vm.count("gameobj") != 1)
    {
        std::cerr << "ERROR: Must specify either a map to generate (--map) or the global generation of BVH data (--gameobj)" << std::endl;
        std::cerr << desc << std::endl;

        return EXIT_FAILURE;
    }

    auto lastStatus = static_cast<time_t>(0);

    parser::Parser::Initialize(dataPath);

    utility::Directory::Create(outputPath);
    utility::Directory::Create(outputPath + "\\BVH");

    if (vm.count("gameobj"))
    {
        GameObjectBVHBuilder goBuilder(outputPath, jobs);

        auto const startSize = goBuilder.Remaining();

        goBuilder.Begin();

        do
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            auto const now = time(nullptr);

            // periodically output current status
            if (now - lastStatus >= STATUS_INTERVAL_SECONDS)
            {
                std::stringstream str;

                auto const percentComplete = static_cast<int>(100.f * static_cast<float>(startSize - goBuilder.Remaining()) / startSize);
                str << "% Complete: " << percentComplete << "\n";
                std::cout << str.str();

                lastStatus = now;
            }

        } while (goBuilder.Remaining() > 0);

        goBuilder.Shutdown();
        goBuilder.WriteIndexFile();

        return EXIT_SUCCESS;
    }

    utility::Directory::Create(outputPath + "\\Nav");

    std::unique_ptr<MeshBuilder> builder;
    std::vector<std::unique_ptr<Worker>> workers;

    if (vm.count("adtX") && vm.count("adtY"))
    {
        builder = std::make_unique<MeshBuilder>(outputPath, map, logLevel, adtX, adtY);

        if (builder->IsGlobalWMO())
        {
            std::cerr << "ERROR: Specified map has no ADTs" << std::endl;
            std::cerr << desc << std::endl;

            return EXIT_FAILURE;
        }

        std::cout << "Building " << map << " (" << adtX << ", " << adtY << ")..." << std::endl;

        workers.push_back(std::make_unique<Worker>(builder.get()));
    }
    // either both, or neither should be specified
    else if (vm.count("adtX") || vm.count("adtY"))
    {
        std::cerr << "ERROR: Must specify ADT X and Y" << std::endl;
        std::cerr << desc << std::endl;

        return EXIT_FAILURE;
    }
    else
    {
        builder = std::make_unique<MeshBuilder>(outputPath, map, logLevel);

        for (auto i = 0; i < jobs; ++i)
            workers.push_back(std::make_unique<Worker>(builder.get()));
    }

    auto const start = time(nullptr);

    for (;;)
    {
        bool done = true;
        for (auto const &worker : workers)
            if (worker->IsRunning())
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

    auto const stop = time(nullptr);
    auto const runTime = stop - start;
    
    std::cout << "Finished " << map << " (" << builder->CompletedTiles() << " tiles) in " << runTime << " seconds." << std::endl;

    return EXIT_SUCCESS;
}