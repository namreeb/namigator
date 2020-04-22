set(EXECUTABLE_NAME MapBuilder)
set(PYTHON_NAME mapbuild)

set(SRC BVHConstructor.cpp GameObjectBVHBuilder.cpp MeshBuilder.cpp RecastContext.cpp Worker.cpp)

add_executable(${EXECUTABLE_NAME} ${SRC} main.cpp)
target_include_directories(${EXECUTABLE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../")
target_link_libraries(${EXECUTABLE_NAME} PRIVATE parser utility RecastNavigation::Recast RecastNavigation::Detour storm ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS ${EXECUTABLE_NAME} DESTINATION bin)

if (PYTHON_COMPONENT)
    if (Python3_FOUND)
        Python3_add_library(${PYTHON_NAME} ${SRC} python.cpp)
    elseif (Python2_FOUND)
        Python2_add_library(${PYTHON_NAME} ${SRC} python.cpp)
    else()
        message(FATAL_ERROR "Python component defined but neither python2 nor python3 were found")
    endif()

    target_compile_definitions(${PYTHON_NAME} PUBLIC PYTHON BOOST_PYTHON_STATIC_LIB)
    target_include_directories(${PYTHON_NAME} PUBLIC ${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})
    target_link_libraries(${PYTHON_NAME} PRIVATE parser utility RecastNavigation::Recast RecastNavigation::Detour storm ${PYTHON_LIBRARIES} ${Boost_LIBRARIES})

    install(TARGETS ${PYTHON_NAME} DESTINATION lib)
endif()
