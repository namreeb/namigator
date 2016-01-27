#include "MeshBuilder.hpp"
#include "Worker.hpp"

#include "utility/Include/Directory.hpp"

#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <thread>
#include <chrono>

int main(int argc, char *argv[])
{
    std::string dataPath, continent, outputPath;
    int adtX, adtY, jobs;

    boost::program_options::options_description desc("Allowed options");
    desc.add_options()
        ("data,d", boost::program_options::value<std::string>(&dataPath)->default_value(".")->required(),           "data folder")
        ("continent,c", boost::program_options::value<std::string>(&continent)->required(),                         "continent")
        ("output,o", boost::program_options::value<std::string>(&outputPath)->default_value(".\\Maps")->required(), "output path")
        ("adtX,x", boost::program_options::value<int>(&adtX),                                                       "adt x")
        ("adtY,y", boost::program_options::value<int>(&adtY),                                                       "adt y")
        ("jobs,j", boost::program_options::value<int>(&jobs)->default_value(8)->required(),                         "build jobs")
        ("help,h",                                                                                                  "display help message");

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

    if (!utility::Directory::Exists(outputPath))
    {
        std::cerr << "ERROR: " << outputPath << " does not exist." << std::endl;
        std::cerr << "Current directory: " << utility::Directory::Current() << std::endl;

        return EXIT_FAILURE;
    }

    MeshBuilder meshBuilder(dataPath, outputPath, continent);

    if (vm.count("adtX") && vm.count("adtY"))
    {
        if (meshBuilder.IsGlobalWMO())
        {
            std::cerr << "ERROR: Specified continent has no ADTs" << std::endl;
            std::cerr << desc << std::endl;

            return EXIT_FAILURE;
        }

        {
            meshBuilder.SingleAdt(adtX, adtY);
        
            std::cout << "Building " << continent << " (" << adtX << ", " << adtY << ")..." << std::endl;
            Worker worker(&meshBuilder);
        }

        std::cout << "Finished.";
        return EXIT_SUCCESS;
    }

    // either both, or neither should be specified
    if (vm.count("adtX") || vm.count("adtY"))
    {
        std::cerr << "ERROR: Must specify ADT X and Y" << std::endl;
        std::cerr << desc << std::endl;

        return EXIT_FAILURE;
    }

    // if the continent is a single wmo, we have no use for multiple threads
    if (meshBuilder.IsGlobalWMO())
    {
        std::cout << "Building global WMO for " << continent << "..." << std::endl;
        Worker worker(&meshBuilder, true);
        return EXIT_SUCCESS;
    }

    // once we reach here it is the usual case of generating an entire continent
    std::vector<std::unique_ptr<Worker>> workers(jobs);

    for (auto &worker : workers)
        worker.reset(new Worker(&meshBuilder));

    auto const start = time(NULL);

    while (true)
    {
#ifdef _DEBUG
        volatile bool writeMemoryUsage = false;

        if (writeMemoryUsage)
        {
            auto const adtMap = meshBuilder.AdtMap();
            auto const adtReferencesMap = meshBuilder.AdtReferencesMap();
            auto const wmoMap = meshBuilder.WmoMap();

            meshBuilder.WriteMemoryUsage(std::cout);
        }
#endif

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
    }

    auto const stop = time(NULL);

    auto const runTime = stop - start;
    auto const adts = meshBuilder.AdtCount();
    auto const secPerAdt = runTime / adts;
    
    std::cout << "Finished " << continent << " (" << adts << " ADTs) in " << runTime << " seconds." << std::endl;

    return EXIT_SUCCESS;
}