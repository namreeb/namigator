set(EXECUTABLE_NAME pathfind_memory_test)

include_directories(Include "${CMAKE_CURRENT_SOURCE_DIR}/../" "${CMAKE_CURRENT_SOURCE_DIR}/../recastnavigation/Detour/Include")

set(SRC
    pathfind_memory_test.cpp
)

add_executable(${EXECUTABLE_NAME} ${SRC})
target_link_libraries(${EXECUTABLE_NAME} PRIVATE libpathfind parser utility recastnavigation storm ${CMAKE_THREAD_LIBS_INIT})
add_dependencies(${EXECUTABLE_NAME} libpathfind parser utility recastnavigation storm)

install(TARGETS ${EXECUTABLE_NAME} DESTINATION bin)
