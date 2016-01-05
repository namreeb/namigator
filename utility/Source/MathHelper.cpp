#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/MathHelper.hpp"

#include <algorithm>
#include <vector>

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

// wow -> r+d: (x, y, z) -> (-y, z, -x)
void Convert::VerticesToRecast(const std::vector<utility::Vertex> &input, std::vector<float> &output)
{
    output.resize(input.size() * 3);

    for (size_t i = 0; i < input.size(); ++i)
    {
        output[i * 3 + 0] = -input[i].Y;
        output[i * 3 + 1] =  input[i].Z;
        output[i * 3 + 2] = -input[i].X;
    }
}

// r+d -> wow: (x, y, z) -> (-z, -x, y)
void Convert::VerticesToWow(const float *input, int vertexCount, std::vector<utility::Vertex> &output)
{
    output.resize(vertexCount);

    float minX, minY, minZ, maxX, maxY, maxZ;

    for (int i = 0; i < vertexCount; ++i)
    {
        output[i].X = -input[i * 3 + 2];
        output[i].Y = -input[i * 3 + 0];
        output[i].Z =  input[i * 3 + 1];

        if (i)
        {
            if (output[i].X < minX)
                minX = output[i].X;
            if (output[i].Y < minY)
                minY = output[i].Y;
            if (output[i].Z < minZ)
                minZ = output[i].Z;

            if (output[i].X > maxX)
                maxX = output[i].X;
            if (output[i].Y > maxY)
                maxY = output[i].Y;
            if (output[i].Z > maxZ)
                maxZ = output[i].Z;
        }
        else
        {
            minX = maxX = output[i].X;
            minY = maxY = output[i].Y;
            minZ = maxZ = output[i].Z;
        }
    }
}

void Convert::ToShort(const std::vector<int> &input, std::vector<unsigned short> &output)
{
    output.resize(input.size());

    for (size_t i = 0; i < input.size(); ++i)
        output[i] = static_cast<unsigned short>(input[i]);
}

void Convert::ToInt(const unsigned short *input, int count, std::vector<int> &output)
{
    output.resize(count);

    for (int i = 0; i < count; ++i)
        output[i] = static_cast<int>(input[i]);
}

void Convert::ToInt(const unsigned char *input, int count, std::vector<int> &output)
{
    output.resize(count);

    for (int i = 0; i < count; ++i)
        output[i] = static_cast<int>(input[i]);
}
}