#include "utility/Include/BoundingBox.hpp"

namespace utility
{
BoundingBox::BoundingBox(const Vector3& min, const Vector3& max) {
    setCorners(min, max);
}

void BoundingBox::transform(const Matrix& mat) {
    float min = std::numeric_limits<float>::lowest();
    float max = std::numeric_limits<float>::max();

    Vector3 newMin = { max, max, max };
    Vector3 newMax = { min, min, min };

    Vector3 corners[8];
    getCorners(corners);

    for (Vector3& v : corners) {
        v = utility::Vertex::Transform(v, mat);
        newMin = takeMinimum(newMin, v);
        newMax = takeMaximum(newMax, v);
    }

    setCorners(newMin, newMax);
}

void BoundingBox::setCorners(const Vector3& min, const Vector3& max) {
    MinCorner = min;
    MaxCorner = max;
}

void BoundingBox::getCorners(Vector3 corners[8]) const {
    corners[0] = { MinCorner.X, MinCorner.Y, MinCorner.Z };
    corners[1] = { MaxCorner.X, MinCorner.Y, MinCorner.Z };
    corners[2] = { MaxCorner.X, MaxCorner.Y, MinCorner.Z };
    corners[3] = { MinCorner.X, MaxCorner.Y, MinCorner.Z };
    corners[4] = { MinCorner.X, MinCorner.Y, MaxCorner.Z };
    corners[5] = { MaxCorner.X, MinCorner.Y, MaxCorner.Z };
    corners[6] = { MaxCorner.X, MaxCorner.Y, MaxCorner.Z };
    corners[7] = { MinCorner.X, MaxCorner.Y, MaxCorner.Z };
}

void BoundingBox::connectWith(const BoundingBox& b) {
    MinCorner = takeMinimum(MinCorner, b.MinCorner);
    MaxCorner = takeMaximum(MaxCorner, b.MaxCorner);
}

float BoundingBox::getVolume() const {
    Vector3 e = MaxCorner - MinCorner;
    return e.X * e.Y * e.Z;
}

float BoundingBox::getSurfaceArea() const {
    Vector3 e = MaxCorner - MinCorner;
    return 2.0f * (e.X*e.Y + e.X*e.Z + e.Y*e.Z);
}

Vector3 BoundingBox::getCenter() const {
    return 0.5f * (MaxCorner - MinCorner);
}

Vector3 BoundingBox::getExtent() const {
    return 0.5f * (MaxCorner - MinCorner);
}

Vector3 BoundingBox::getVector() const {
    return MaxCorner - MinCorner;
}

const Vector3& BoundingBox::getMinimum() const {
    return MinCorner;
}

const Vector3& BoundingBox::getMaximum() const {
    return MaxCorner;
}
}