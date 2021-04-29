#pragma once

#include "utility/Vector.hpp"

#include <memory>
#include <vector>

namespace math
{
class MathHelper
{
public:
    static bool FaceTooSteep(const Vector3& a, const Vector3& b,
                             const Vector3& c, float degrees);
    static float InterpolateHeight(const Vector3& a, const Vector3& b,
                                   const Vector3& c, float x, float y);

    static Vector3 CalculateTriangleNormal(const Vector3 a, const Vector3 b,
                                           const Vector3 c)
    {
        const Vector3 ab(b.X - a.X, b.Y - a.Y, b.Z - a.Z),
            ac(c.X - a.X, c.Y - a.Y, c.Z - a.Z);
        const Vector3 n = Vector3::CrossProduct(ab, ac);

        return Vector3::Normalize(n);
    }
};

class Convert
{
public:
    static float ToRadians(float degrees);

    static void WorldToAdt(const Vector3& vertex, int& adtX, int& adtY);
    static void WorldToAdt(const Vector3& vertex, int& adtX, int& adtY,
                           int& chunkX, int& chunkY);
    static void WorldToTile(const Vector3& vertex, int& tileX, int& tileY);

    static void ADTToWorldNorthwestCorner(int adtX, int adtY, float& worldX,
                                          float& worldY);
    static void TileToWorldNorthwestCorner(int tileX, int tileY, float& worldX,
                                           float& worldY);

    static void VertexToRecast(const Vector3& input, Vector3& output);
    static void VertexToRecast(const Vector3& input, float* output);
    static void VerticesToRecast(const std::vector<Vector3>& input,
                                 std::vector<float>& output);

    static void VertexToWow(const float* input, Vector3& output);
    static void VerticesToWow(const float* input, int vertexCount,
                              std::vector<Vector3>& output);
};
} // namespace math