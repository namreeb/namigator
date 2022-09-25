#include "utility/BoundingBox.hpp"

#include <algorithm>
#include <limits>

namespace math
{
BoundingBox::BoundingBox(const math::Vertex& min, const math::Vertex& max)
{
    setCorners(min, max);
}

void BoundingBox::transform(const math::Matrix& mat)
{
    float min = std::numeric_limits<float>::lowest();
    float max = std::numeric_limits<float>::max();

    math::Vertex newMin = {max, max, max};
    math::Vertex newMax = {min, min, min};

    math::Vertex corners[8];
    getCorners(corners);

    for (auto& v : corners)
    {
        v = math::Vertex::Transform(v, mat);
        newMin = takeMinimum(newMin, v);
        newMax = takeMaximum(newMax, v);
    }

    setCorners(newMin, newMax);
}

void BoundingBox::setCorners(const math::Vertex& min, const math::Vertex& max)
{
    MinCorner = min;
    MaxCorner = max;
}

void BoundingBox::getCorners(math::Vertex corners[8]) const
{
    corners[0] = {MinCorner.X, MinCorner.Y, MinCorner.Z};
    corners[1] = {MaxCorner.X, MinCorner.Y, MinCorner.Z};
    corners[2] = {MaxCorner.X, MaxCorner.Y, MinCorner.Z};
    corners[3] = {MinCorner.X, MaxCorner.Y, MinCorner.Z};
    corners[4] = {MinCorner.X, MinCorner.Y, MaxCorner.Z};
    corners[5] = {MaxCorner.X, MinCorner.Y, MaxCorner.Z};
    corners[6] = {MaxCorner.X, MaxCorner.Y, MaxCorner.Z};
    corners[7] = {MinCorner.X, MaxCorner.Y, MaxCorner.Z};
}

void BoundingBox::connectWith(const BoundingBox& b)
{
    MinCorner = takeMinimum(MinCorner, b.MinCorner);
    MaxCorner = takeMaximum(MaxCorner, b.MaxCorner);
}

void BoundingBox::update(const math::Vector3& v)
{
    MinCorner.X = (std::min)(MinCorner.X, v.X);
    MaxCorner.X = (std::max)(MaxCorner.X, v.X);
    MinCorner.Y = (std::min)(MinCorner.Y, v.Y);
    MaxCorner.Y = (std::max)(MaxCorner.Y, v.Y);
    MinCorner.Z = (std::min)(MinCorner.Z, v.Z);
    MaxCorner.Z = (std::max)(MaxCorner.Z, v.Z);
}

float BoundingBox::getVolume() const
{
    math::Vector3 e = MaxCorner - MinCorner;
    return e.X * e.Y * e.Z;
}

float BoundingBox::getSurfaceArea() const
{
    math::Vector3 e = MaxCorner - MinCorner;
    return 2.0f * (e.X * e.Y + e.X * e.Z + e.Y * e.Z);
}

math::Vector3 BoundingBox::getCenter() const
{
    return 0.5f * (MaxCorner + MinCorner);
}

math::Vector3 BoundingBox::getExtent() const
{
    return 0.5f * (MaxCorner - MinCorner);
}

math::Vector3 BoundingBox::getVector() const
{
    return MaxCorner - MinCorner;
}

const math::Vector3& BoundingBox::getMinimum() const
{
    return MinCorner;
}

const math::Vector3& BoundingBox::getMaximum() const
{
    return MaxCorner;
}

bool BoundingBox::intersect2d(const BoundingBox& b) const
{
    if (MaxCorner.X < b.MinCorner.X)
        return false;
    if (MinCorner.X > b.MaxCorner.X)
        return false;

    if (MaxCorner.Y < b.MinCorner.Y)
        return false;
    if (MinCorner.Y > b.MaxCorner.Y)
        return false;

    return true;
}

bool BoundingBox::intersect(const BoundingBox& b) const
{
    if (!intersect2d(b))
        return false;

    if (MaxCorner.Z < b.MinCorner.Z)
        return false;
    if (MinCorner.Z > b.MaxCorner.Z)
        return false;

    return true; // boxes overlap
}

std::ostream& operator<<(std::ostream& o, const BoundingBox& b)
{
    o << b.MinCorner << b.MaxCorner;
    return o;
}

std::istream& operator>>(std::istream& i, BoundingBox& b)
{
    i >> b.MinCorner >> b.MaxCorner;
    return i;
}
} // namespace math