#include "pathfind/Map.hpp"

#include <experimental/filesystem>
#include <iostream>

#include <Windows.h>

namespace fs = std::experimental::filesystem;

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <data path>" << std::endl;
        return EXIT_FAILURE;
    }

    const fs::path data_path(argv[1]);

    if (!fs::is_directory(data_path))
    {
        std::cerr << "Data directory " << data_path.string() << " does not exist." << std::endl;
        return EXIT_FAILURE;
    }

    pathfind::Map azeroth(data_path.string(), "Azeroth");

    azeroth.LoadAllADTs();

    return EXIT_SUCCESS;
}