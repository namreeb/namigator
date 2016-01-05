#pragma once

#include "Output/Continent.hpp"

#include <string>
#include <memory>

namespace pathfind
{
    namespace build
    {
        class DataManager
        {
            friend class MeshBuilder;

            private:
                std::unique_ptr<parser::Continent> m_continent;

            public:
                DataManager(const std::string &continentName);
        };
    }
}