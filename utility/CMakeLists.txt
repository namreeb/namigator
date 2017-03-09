include_directories(Include ${CMAKE_SOURCE_DIR})

add_library(utility STATIC
    Source/AABBTree.cpp
    Source/BinaryStream.cpp
    Source/BoundingBox.cpp
    Source/LinearAlgebra.cpp
    Source/MathHelper.cpp
    Source/Ray.cpp
)