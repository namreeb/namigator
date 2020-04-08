set(LIBRARY_NAME pathfind)

include_directories(Include ${CMAKE_SOURCE_DIR})

set(SRC
    BVH.cpp
    Map.cpp
    TemporaryObstacle.cpp
    Tile.cpp
)

add_library(lib${LIBRARY_NAME} STATIC ${SRC})

add_dependencies(lib${LIBRARY_NAME} utility recastnavigation)

if (PYTHON_COMPONENT)
    if (Python3_FOUND)
        Python3_add_library(${LIBRARY_NAME} ${SRC} python.cpp)
    elseif (Python2_FOUND)
        Python2_add_library(${LIBRARY_NAME} ${SRC} python.cpp)
    else()
        message(FATAL_ERROR "Python component defined but neither python2 nor python3 were found")
    endif()

    target_compile_definitions(${LIBRARY_NAME} PUBLIC PYTHON BOOST_PYTHON_STATIC_LIB)
    include_directories(${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})
    add_dependencies(${LIBRARY_NAME} utility recastnavigation storm)
    target_link_libraries(${LIBRARY_NAME} PRIVATE utility recastnavigation storm ${Boost_LIBRARIES})

    install(TARGETS ${LIBRARY_NAME} DESTINATION lib)
endif()