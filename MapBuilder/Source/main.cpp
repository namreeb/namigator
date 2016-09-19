#include "MeshBuilder.hpp"
#include "Worker.hpp"

#include "parser/Include/Adt/Adt.hpp"
#include "parser/Include/Wmo/WmoInstance.hpp"

#include "utility/Include/Directory.hpp"

#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <thread>
#include <chrono>

int main(int argc, char *argv[])
{
    std::string dataPath, map, outputPath;
    int adtX, adtY, jobs, logLevel;

    boost::program_options::options_description desc("Allowed options");
    desc.add_options()
        ("data,d", boost::program_options::value<std::string>(&dataPath)->default_value("."),           "data folder")
        ("map,m", boost::program_options::value<std::string>(&map)->required(),                         "map")
        ("output,o", boost::program_options::value<std::string>(&outputPath)->default_value(".\\Maps"), "output path")
        ("adtX,x", boost::program_options::value<int>(&adtX),                                           "adt x")
        ("adtY,y", boost::program_options::value<int>(&adtY),                                           "adt y")
        ("jobs,j", boost::program_options::value<int>(&jobs)->default_value(8),                         "build jobs")
        ("logLevel,l", boost::program_options::value<int>(&logLevel)->default_value(3),                 "log level (0 = none, 1 = progress, 2 = warning, 3 = error)")
        ("gset,g",                                                                                      "dump .gset file instead of mesh")
        ("help,h",                                                                                      "display help message");

    boost::program_options::variables_map vm;

    try
    {
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
        boost::program_options::notify(vm);

        if (vm.count("help"))
        {
            std::cout << desc << std::endl;
            return EXIT_SUCCESS;
        }
    }
    catch (boost::program_options::error const &e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
        std::cerr << desc << std::endl;

        return EXIT_FAILURE;
    }

    utility::Directory::Create(outputPath);
    utility::Directory::Create(outputPath + "\\BVH");
    utility::Directory::Create(outputPath + "\\Nav");

    MeshBuilder meshBuilder(dataPath, outputPath, map, logLevel);

    if (vm.count("gset"))
    {
        if (meshBuilder.IsGlobalWMO())
        {
            if (!meshBuilder.GenerateAndSaveGSet())
            {
                std::cerr << "ERROR: Failed to save global .gset file" << std::endl;
                return EXIT_FAILURE;
            }

            std::cout << "Global .gset file written succesfully" << std::endl;

            return EXIT_SUCCESS;
        }

        if (!vm.count("tileX") || !vm.count("tileY"))
        {
            std::cerr << "ERROR: --gset requires --tileX and --tileY" << std::endl << std::endl;
            std::cerr << desc << std::endl;

            return EXIT_FAILURE;
        }

        //if (!meshBuilder.GenerateAndSaveGSet(adtX, adtY))
        //{
        //    std::cerr << "ERROR: Failed to save ADT (" << adtX << ", " << adtY << ") .gset file" << std::endl;
        //    return EXIT_FAILURE;
        //}

        return EXIT_SUCCESS;
    }

    if (vm.count("tileX") && vm.count("tileY"))
    {
        if (meshBuilder.IsGlobalWMO())
        {
            std::cerr << "ERROR: Specified Map has no ADTs" << std::endl;
            std::cerr << desc << std::endl;

            return EXIT_FAILURE;
        }

        {
            meshBuilder.SingleADT(adtX, adtY);
        
            std::cout << "Building " << map << " (" << adtX << ", " << adtY << ")..." << std::endl;
            Worker worker(&meshBuilder);
        }

        std::cout << "Finished.";
        return EXIT_SUCCESS;
    }

    // either both, or neither should be specified
    if (vm.count("tileX") || vm.count("tileY"))
    {
        std::cerr << "ERROR: Must specify tile X and Y" << std::endl;
        std::cerr << desc << std::endl;

        return EXIT_FAILURE;
    }

    // if the Map is a single wmo, we have no use for multiple threads
    if (meshBuilder.IsGlobalWMO())
    {
        std::cout << "Building global WMO for " << map << "..." << std::endl;
        {
            Worker worker(&meshBuilder, true);
        }

        meshBuilder.SaveMap();
        return EXIT_SUCCESS;
    }

    // once we reach here it is the usual case of generating an entire Map
    std::vector<std::unique_ptr<Worker>> workers(jobs);

    for (auto &worker : workers)
        worker.reset(new Worker(&meshBuilder));

    auto const start = time(nullptr);

    auto lastStatus = static_cast<time_t>(0);

    for (;;)
    {
        bool done = true;
        for (auto &worker : workers)
            if (worker->IsRunning())
            {
                done = false;
                break;
            }

        if (done)
            break;

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        auto const now = time(nullptr);

        // every 10 seconds, output current status
        if (now - lastStatus >= 10)
        {
            std::stringstream str;

            str << "% Complete: " << meshBuilder.PercentComplete() << "\n";
            std::cout << str.str();

            lastStatus = now;
        }
    }

    meshBuilder.SaveMap();

    auto const stop = time(nullptr);

    auto const runTime = stop - start;
    auto const tiles = meshBuilder.TotalTiles();
    
    std::cout << "Finished " << map << " (" << tiles << " tiles) in " << runTime << " seconds." << std::endl;

    return EXIT_SUCCESS;
}