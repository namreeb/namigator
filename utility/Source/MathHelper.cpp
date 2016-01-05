#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/MathHelper.hpp"

#include <algorithm>

namespace utility
{
float MathHelper::ToRadians(float degrees)
{
    const float conversion = (float)(PI / 180.f);

    return degrees * conversion;
}

bool MathHelper::FaceTooSteep(const utility::Vertex &a, const utility::Vertex &b, const utility::Vertex &c, float degrees)
{
    return CalculateTriangleNormal(a, b, c).Z <= cosf(PI * degrees / 180.f);
}

float MathHelper::InterpolateHeight(const utility::Vertex &a, const utility::Vertex &b, const utility::Vertex &c, float x, float y)
{
    // we have the vertices a, b and c for the triangle the point falls on.  
    // we compute ab (b - a) and ac (c - a), determine ab x ac, giving us a std::vector normal to the plane,
    // then we have an equation for the plane which we can solve for the desired z value.
    Vector3 ab(b.X - a.X, b.Y - a.Y, b.Z - a.Z), ac(c.X - a.X, c.Y - a.Y, c.Z - a.Z);
    Vector3 n = Vector3::CrossProduct(ab, ac);

    // re-arranging equation for plane: n_x(x - x_o) + n_y(y - y_0) + n_z(z - z_0) = 0, solve for z.
    return a.Z + (-n.X * (x - a.X) - n.Y * (y - a.Y)) / n.Z;
}

int MathHelper::Round(float num)
{
    return (int)floorf(num + 0.5f);
}

//BoundingBox::BoundingBox()
//    : MinCorner(0.f, 0.f, 0.f), MaxCorner(0.f, 0.f, 0.f) {}

//BoundingBox::BoundingBox(const utility::Vertex &minCorner, const utility::Vertex &maxCorner)
//    : MinCorner(minCorner), MaxCorner(maxCorner) {}

//BoundingBox::BoundingBox(const utility::Vertex &a, const utility::Vertex &b, const utility::Vertex &c)
//    : MinCorner(std::min({ a.X, b.X, c.X }), std::min({ a.Y, b.Y, c.Y }), std::min({ a.Z, b.Z, c.Z })),
//      MaxCorner(std::max({ a.X, b.X, c.X }), std::max({ a.Y, b.Y, c.Y }), std::max({ a.Z, b.Z, c.Z })) {}

//BoundingBox::BoundingBox(float minX, float minY, float maxX, float maxY)
//    : MinCorner(minX, maxX, 0.f), MaxCorner(maxX, maxY, 0.f) {}

//BoundingBox::BoundingBox(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
//    : MinCorner(minX, minY, minZ), MaxCorner(maxX, maxY, maxZ) {}

//bool BoundingBox::Intersects(const BoundingBox &other) const
//{
//    return !(MinCorner.X > other.MaxCorner.X || MaxCorner.X < other.MinCorner.X ||
//             MinCorner.Y > other.MaxCorner.Y || MaxCorner.Y < other.MinCorner.Y ||
//             MinCorner.Z > other.MaxCorner.Z || MaxCorner.Z < other.MinCorner.Z);
//}

//bool BoundingBox::Contains(const utility::Vertex &vertex) const
//{
//    return !(vertex.X < MinCorner.X || vertex.X > MaxCorner.X ||
//             vertex.Y < MinCorner.Y || vertex.Y > MaxCorner.Y ||
//             vertex.Z < MinCorner.Z || vertex.Z > MaxCorner.Z);
//}

void Convert::AdtToWorldNorthwestCorner(int adtX, int adtY, float &worldX, float &worldY)
{
    worldX = ((float)(32.f - adtY)) * (533.f + (1.f / 3.f));
    worldY = ((float)(32.f - adtX)) * (533.f + (1.f / 3.f));
}
}