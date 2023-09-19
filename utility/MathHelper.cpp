#include "utility/MathHelper.hpp"

#include "Common.hpp"
#include "utility/Vector.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

/*
 *        +X
 *     +Y    -Y
 *        -X
 *  When you go up, your ingame X goes up, but the ADT file Y goes down.
 *  When you go left, your ingame Y goes up, but the ADT file X goes down.
 *  When you go down, your ingame X goes down, but the ADT file Y goes up.
 *  When you go right, your ingame Y goes down, but the ADT file X goes up.
 */

namespace math
{
bool MathHelper::FaceTooSteep(const Vector3& a, const Vector3& b,
                              const Vector3& c, float degrees)
{
    return CalculateTriangleNormal(a, b, c).Z <= cosf(PI * degrees / 180.f);
}

float MathHelper::InterpolateHeight(const Vector3& a, const Vector3& b,
                                    const Vector3& c, float x, float y)
{
    // we have the vertices a, b and c for the triangle the point falls on.
    // we compute ab (b - a) and ac (c - a), determine ab x ac, giving us a
    // std::vector normal to the plane, then we have an equation for the plane
    // which we can solve for the desired z value.
    Vector3 ab(b.X - a.X, b.Y - a.Y, b.Z - a.Z),
        ac(c.X - a.X, c.Y - a.Y, c.Z - a.Z);
    Vector3 n = Vector3::CrossProduct(ab, ac);

    // re-arranging equation for plane: n_x(x - x_o) + n_y(y - y_0) + n_z(z -
    // z_0) = 0, solve for z.
    return a.Z + (-n.X * (x - a.X) - n.Y * (y - a.Y)) / n.Z;
}

// BoundingBox::BoundingBox()
//    : MinCorner(0.f, 0.f, 0.f), MaxCorner(0.f, 0.f, 0.f) {}

// BoundingBox::BoundingBox(const Vector3 &minCorner, const Vector3 &maxCorner)
//    : MinCorner(minCorner), MaxCorner(maxCorner) {}

// BoundingBox::BoundingBox(const Vector3 &a, const Vector3 &b, const Vector3
// &c)
//    : MinCorner(std::min({ a.X, b.X, c.X }), std::min({ a.Y, b.Y, c.Y }),
//    std::min({ a.Z, b.Z, c.Z })),
//      MaxCorner(std::max({ a.X, b.X, c.X }), std::max({ a.Y, b.Y, c.Y }),
//      std::max({ a.Z, b.Z, c.Z })) {}

// BoundingBox::BoundingBox(float minX, float minY, float maxX, float maxY)
//    : MinCorner(minX, maxX, 0.f), MaxCorner(maxX, maxY, 0.f) {}

// BoundingBox::BoundingBox(float minX, float minY, float minZ, float maxX,
// float maxY, float maxZ)
//    : MinCorner(minX, minY, minZ), MaxCorner(maxX, maxY, maxZ) {}

// bool BoundingBox::Intersects(const BoundingBox &other) const
//{
//    return !(MinCorner.X > other.MaxCorner.X || MaxCorner.X <
//    other.MinCorner.X ||
//             MinCorner.Y > other.MaxCorner.Y || MaxCorner.Y <
//             other.MinCorner.Y || MinCorner.Z > other.MaxCorner.Z ||
//             MaxCorner.Z < other.MinCorner.Z);
//}

// bool BoundingBox::Contains(const Vector3 &Vector3) const
//{
//    return !(Vector3.X < MinCorner.X || Vector3.X > MaxCorner.X ||
//             Vector3.Y < MinCorner.Y || Vector3.Y > MaxCorner.Y ||
//             Vector3.Z < MinCorner.Z || Vector3.Z > MaxCorner.Z);
//}

float Convert::ToRadians(float degrees)
{
    auto constexpr conversion = PI / 180.f;
    return degrees * conversion;
}

void Convert::WorldToAdt(const Vector3& vertex, int& adtX, int& adtY)
{
    // FIXME: We can't use MeshSettings::AdtSize here because of floating point
    // imprecision.  Should consider changing MeshSettings:: values to double.
    auto constexpr mid = 32.0 * (533.0 + (1.0 / 3.0));

    adtX = static_cast<int>((mid - vertex.Y) / MeshSettings::AdtSize);
    adtY = static_cast<int>((mid - vertex.X) / MeshSettings::AdtSize);
}

// chunk (0, 0) is at the top left (northwest) corner.  east = x+, south = y+
void Convert::WorldToAdt(const Vector3& vertex, int& adtX, int& adtY,
                         int& chunkX, int& chunkY)
{
    WorldToAdt(vertex, adtX, adtY);

    float nwCornerX, nwCornerY;
    ADTToWorldNorthwestCorner(adtX, adtY, nwCornerX, nwCornerY);

    chunkX =
        static_cast<int>((nwCornerY - vertex.Y) / MeshSettings::AdtChunkSize);
    chunkY =
        static_cast<int>((nwCornerX - vertex.X) / MeshSettings::AdtChunkSize);

    assert(chunkX >= 0 && chunkY >= 0 && chunkX < MeshSettings::ChunksPerAdt &&
           chunkY < MeshSettings::ChunksPerAdt);
}

void Convert::WorldToTile(const Vector3& vertex, int& tileX, int& tileY)
{
    auto constexpr start = (MeshSettings::Adts / 2.0) * MeshSettings::AdtSize;
    tileX = static_cast<int>((start - vertex.Y) / MeshSettings::TileSize);
    tileY = static_cast<int>((start - vertex.X) / MeshSettings::TileSize);
}

void Convert::ADTToWorldNorthwestCorner(int adtX, int adtY, float& worldX,
                                        float& worldY)
{
    worldX = (32.f - adtY) * MeshSettings::AdtSize;
    worldY = (32.f - adtX) * MeshSettings::AdtSize;
}

void Convert::TileToWorldNorthwestCorner(int tileX, int tileY, float& worldX,
                                         float& worldY)
{
    auto constexpr start = (MeshSettings::Adts / 2.0) * MeshSettings::AdtSize;
    worldX = static_cast<float>(start - tileY * MeshSettings::TileSize);
    worldY = static_cast<float>(start - tileX * MeshSettings::TileSize);
}

void Convert::VertexToRecast(const Vector3& input, Vector3& output)
{
    VertexToRecast(input, reinterpret_cast<float*>(&output));
}

void Convert::VertexToRecast(const Vector3& input, float* output)
{
    assert(!!output);

    // this is necessary in case input = output
    const float x = input.X;

    output[0] = -input.Y;
    output[1] = input.Z;
    output[2] = -x;
}

void Convert::VerticesToRecast(const std::vector<Vector3>& input,
                               std::vector<float>& output)
{
    output.resize(input.size() * 3);

    for (size_t i = 0; i < input.size(); ++i)
        VertexToRecast(input[i], &output[i * 3]);
}

void Convert::VertexToWow(const float* input, Vector3& output)
{
    // this is necessary in case input = output
    const float x = input[1];

    output.X = -input[2];
    output.Y = -input[0];
    output.Z = x;
}

void Convert::VerticesToWow(const float* input, int Vector3Count,
                            std::vector<Vector3>& output)
{
    output.resize(Vector3Count);

    for (int i = 0; i < Vector3Count; ++i)
        VertexToWow(&input[i * 3], output[i]);
}
} // namespace math