add_library(utility STATIC
    AABBTree.cpp
    BinaryStream.cpp
    BoundingBox.cpp
    Matrix.cpp
    Vector.cpp
    Quaternion.cpp
    MathHelper.cpp
    Ray.cpp
    String.cpp
)

target_include_directories(utility PUBLIC ..)

if (NAMIGATOR_INSTALL_NON_PYTHON_FILES)
    install(TARGETS utility ARCHIVE DESTINATION lib)
endif()
