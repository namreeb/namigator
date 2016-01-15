#pragma once

#include "utility/Include/LinearAlgebra.hpp"

namespace utility
{
class BoundingBox
{
    public:
        BoundingBox() = default;
        ~BoundingBox() = default;

        BoundingBox(const Vector3& min, const Vector3& max);

        void transform(const Matrix& mat);
        void connectWith(const BoundingBox& b);

        void setCorners(const Vector3& min, const Vector3& max);
        void getCorners(Vector3 corners[8]) const;

        float getVolume() const;
        float getSurfaceArea() const;

        Vector3 getCenter() const;
        Vector3 getExtent() const;
        Vector3 getVector() const;

        const Vector3& getMinimum() const;
        const Vector3& getMaximum() const;

        Vector3 MinCorner, MaxCorner;
};
}