#pragma once

#include <memory>
#include <algorithm>

#include "LinearAlgebra.hpp"

#ifndef PI
#define PI 3.14159264f
#endif

#define MIN3(a, b, c) (a < b ? std::min(a, c) : std::min(b, c))
#define MAX3(a, b, c) (a > b ? std::max(a, c) : std::max(b, c))

namespace utility
{
    class MathHelper
    {
        public:
            static const float ToRadians(const float degrees);
            static const bool FaceTooSteep(const Vertex &a, const Vertex &b, const Vertex &c, const float degrees);
            static const float InterpolateHeight(const Vertex &a, const Vertex &b, const Vertex &c, const float x, const float y);
            static const int Round(const float num);

            template <typename T>
            static const Vector3<T> CalculateTriangleNormal(const Vector3<T> a, const Vector3<T> b, const Vector3<T> c)
            {
                Vector3<T> ab(b.X - a.X, b.Y - a.Y, b.Z - a.Z), ac(c.X - a.X, c.Y - a.Y, c.Z - a.Z);
                Vector3<T> n = Vector3<T>::CrossProduct(ab, ac);

                return Vector3<T>::Normalize(n);
            }
    };

    class BoundingBox
    {
        private:
            bool IncludeZ;

        public:
            Vertex MinCorner;
            Vertex MaxCorner;

            BoundingBox();
            BoundingBox(const Vertex &minCorner, const Vertex &maxCorner, bool includeZ = true);
            BoundingBox(Vertex &a, Vertex &b, Vertex &c, bool includeZ = true);
            BoundingBox(float minX, float minY, float maxX, float maxY);
            BoundingBox(float minX, float minY, float minZ, float maxX, float maxY, float maxZ, bool includeZ = true);

            bool Intersects(const BoundingBox &other);
            bool Contains(const Vertex &vertex);
    };

    class Convert
    {
        public:
            static void AdtToWorldNorthwestCorner(int adtX, int adtY, float &worldX, float &worldY);
    };
}