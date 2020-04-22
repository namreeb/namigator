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

target_include_directories(utility PUBLIC ..)