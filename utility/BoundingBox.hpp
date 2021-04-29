#pragma once

#include "utility/Vector.hpp"

#include <ostream>

namespace math
{
class BoundingBox
{
public:
    BoundingBox() = default;
    ~BoundingBox() = default;

    BoundingBox(const Vertex& min, const Vertex& max);

    void transform(const Matrix& mat);
    void connectWith(const BoundingBox& b);
    void update(const Vector3& v);

    void setCorners(const Vertex& min, const Vertex& max);
    void getCorners(Vertex corners[8]) const;

    float getVolume() const;
    float getSurfaceArea() const;

    Vertex getCenter() const;
    Vector3 getExtent() const;
    Vector3 getVector() const;

    const Vertex& getMinimum() const;
    const Vertex& getMaximum() const;

    bool intersect2d(const BoundingBox& b) const;
    bool intersect(const BoundingBox& b) const;

    Vector3 MinCorner, MaxCorner;
};

std::ostream& operator<<(std::ostream&, const BoundingBox&);
std::istream& operator>>(std::istream&, BoundingBox&);
} // namespace math