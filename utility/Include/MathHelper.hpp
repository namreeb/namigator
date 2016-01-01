#pragma once

#include <memory>

#include "LinearAlgebra.hpp"

#ifndef PI
#define PI 3.14159264f
#endif

namespace utility
{
    class MathHelper
    {
        public:
            static float ToRadians(float degrees);
            static bool FaceTooSteep(const Vertex &a, const Vertex &b, const Vertex &c, float degrees);
            static float InterpolateHeight(const Vertex &a, const Vertex &b, const Vertex &c, float x, float y);
            static int Round(float num);

            static Vector3 CalculateTriangleNormal(const Vector3 a, const Vector3 b, const Vector3 c)
            {
                const Vector3 ab(b.X - a.X, b.Y - a.Y, b.Z - a.Z), ac(c.X - a.X, c.Y - a.Y, c.Z - a.Z);
                const Vector3 n = Vector3::CrossProduct(ab, ac);

                return Vector3::Normalize(n);
            }
    };

    //class BoundingBox
    //{
    //    public:
    //        Vertex MinCorner;
    //        Vertex MaxCorner;

    //        BoundingBox();
    //        BoundingBox(const Vertex &minCorner, const Vertex &maxCorner);
    //        BoundingBox(const Vertex &a, const Vertex &b, const Vertex &c);
    //        BoundingBox(float minX, float minY, float maxX, float maxY);
    //        BoundingBox(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);

    //        bool Intersects(const BoundingBox &other) const;
    //        bool Contains(const Vertex &vertex) const;
    //};

    class Convert
    {
        public:
            static void AdtToWorldNorthwestCorner(int adtX, int adtY, float &worldX, float &worldY);
    };
}