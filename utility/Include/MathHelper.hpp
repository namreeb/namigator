#pragma once

#include "utility/Include/LinearAlgebra.hpp"

#include <memory>
#include <vector>

#ifndef PI
#define PI 3.14159264f
#endif

namespace utility
{
class MathHelper
{
    public:
        static float ToRadians(float degrees);
        static bool FaceTooSteep(const utility::Vertex &a, const utility::Vertex &b, const utility::Vertex &c, float degrees);
        static float InterpolateHeight(const utility::Vertex &a, const utility::Vertex &b, const utility::Vertex &c, float x, float y);
        static int Round(float num);

        static Vector3 CalculateTriangleNormal(const Vector3 a, const Vector3 b, const Vector3 c)
        {
            const Vector3 ab(b.X - a.X, b.Y - a.Y, b.Z - a.Z), ac(c.X - a.X, c.Y - a.Y, c.Z - a.Z);
            const Vector3 n = Vector3::CrossProduct(ab, ac);

            return Vector3::Normalize(n);
        }
};

class Convert
{
    public:
        static void AdtToWorldNorthwestCorner(int adtX, int adtY, float &worldX, float &worldY);

        static void VerticesToRecast(const std::vector<utility::Vertex> &input, std::vector<float> &output);
        static void VerticesToWow(const float *input, int vertexCount, std::vector<utility::Vertex> &output);

        // XXX FIXME we maybe never need to change the indices from ushort to int in the first place?
        static void ToShort(const std::vector<int> &input, std::vector<unsigned short> &output);
        static void ToInt(const unsigned short *input, int count, std::vector<int> &output);
        static void ToInt(const unsigned char *input, int count, std::vector<int> &output);
};
}