include_directories(Include "${CMAKE_CURRENT_SOURCE_DIR}/../")

add_library(utility STATIC
    AABBTree.cpp
    BinaryStream.cpp
    BoundingBox.cpp
    Matrix.cpp
    Vector.cpp
    Quaternion.cpp
    MathHelper.cpp
    Ray.cpp
)