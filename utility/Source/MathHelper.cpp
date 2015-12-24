#include "LinearAlgebra.hpp"
#include "MathHelper.hpp"

namespace utility
{
    const float MathHelper::ToRadians(const float degrees)
    {
        const float conversion = (float)(PI / 180.0);

        return degrees * conversion;
    }
    
    const bool MathHelper::FaceTooSteep(const Vertex &a, const Vertex &b, const Vertex &c, const float degrees)
    {
        return CalculateTriangleNormal(a, b, c).Z <= cosf(PI * degrees / 180.f);
    }

    const float MathHelper::InterpolateHeight(const Vertex &a, const Vertex &b, const Vertex &c, const float x, const float y)
    {
        // we have the vertices a, b and c for the triangle the point falls on.  
        // we compute ab (b - a) and ac (c - a), determine ab x ac, giving us a std::vector normal to the plane,
        // then we have an equation for the plane which we can solve for the desired z value.
        Vector3<float> ab(b.X - a.X, b.Y - a.Y, b.Z - a.Z), ac(c.X - a.X, c.Y - a.Y, c.Z - a.Z);
        Vector3<float> n = Vector3<float>::CrossProduct(ab, ac);

        // re-arranging equation for plane: n_x(x - x_o) + n_y(y - y_0) + n_z(z - z_0) = 0, solve for z.
        return a.Z + (-n.X * (x - a.X) - n.Y * (y - a.Y)) / n.Z;
    }

    const int MathHelper::Round(const float num)
    {
        return (int)floor(num + 0.5f);
    }

    BoundingBox::BoundingBox()
    {
    }

    BoundingBox::BoundingBox(const Vertex &minCorner, const Vertex &maxCorner, bool includeZ)
    {
        MinCorner = minCorner;
        MaxCorner = maxCorner;
        IncludeZ = includeZ;
    }

    BoundingBox::BoundingBox(Vertex &a, Vertex &b, Vertex &c, bool includeZ)
    {
        MinCorner.X = MIN3(a.X, b.X, c.X);
        MinCorner.Y = MIN3(a.Y, b.Y, c.Y);
        MinCorner.Z = MIN3(a.Z, b.Z, c.Z);

        MaxCorner.X = MAX3(a.X, b.X, c.X);
        MaxCorner.Y = MAX3(a.Y, b.Y, c.Y);
        MaxCorner.Z = MAX3(a.Z, b.Z, c.Z);

        IncludeZ = includeZ;
    }

    BoundingBox::BoundingBox(float minX, float minY, float maxX, float maxY)
    {
        MinCorner = Vertex(minX, minY, 0.f);
        MaxCorner = Vertex(maxX, maxY, 0.f);
        IncludeZ = false;
    }

    BoundingBox::BoundingBox(float minX, float minY, float minZ, float maxX, float maxY, float maxZ, bool includeZ)
    {
        MinCorner = Vertex(minX, minY, minZ);
        MaxCorner = Vertex(maxX, maxY, maxZ);
        IncludeZ = includeZ;
    }

    bool BoundingBox::Intersects(const BoundingBox &other)
    {
        return !(MinCorner.X > other.MaxCorner.X || MaxCorner.X < other.MinCorner.X ||
                 MinCorner.Y > other.MaxCorner.Y || MaxCorner.Y < other.MinCorner.Y ||
                 (IncludeZ && other.IncludeZ && (MinCorner.Z > other.MaxCorner.Z || MaxCorner.Z < other.MinCorner.Z)));
    }

    bool BoundingBox::Contains(const Vertex &vertex)
    {
        return !(vertex.X < MinCorner.X || vertex.X > MaxCorner.X ||
                 vertex.Y < MinCorner.Y || vertex.Y > MaxCorner.Y ||
                 (IncludeZ && (vertex.Z < MinCorner.Z || vertex.Z > MaxCorner.Z)));
    }

    void Convert::AdtToWorldNorthwestCorner(int adtX, int adtY, float &worldX, float &worldY)
    {
        worldX = ((float)(32 - adtY)) * (533.f + (1.f / 3.f));
        worldY = ((float)(32 - adtX)) * (533.f + (1.f / 3.f));
    }
}