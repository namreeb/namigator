#pragma once

#include <algorithm>
#include <iostream>

#ifndef PI
#    define PI 3.14159264f
#endif

namespace math
{
struct Vector2
{
    const float X;
    const float Y;

    Vector2(float x, float y) : X(x), Y(y) {}
};

class Matrix;

struct Vector3
{
    static float DotProduct(const Vector3& a, const Vector3& b);
    static Vector3 CrossProduct(const Vector3& a, const Vector3& b);
    static Vector3 Normalize(const Vector3& a);
    static Vector3 Transform(const Vector3& position, const Matrix& matrix);
    float GetDistance(const Vector3& other) const;

    float X;
    float Y;
    float Z;

    Vector3() : X(0.f), Y(0.f), Z(0.f) {}
    Vector3(float x, float y, float z) : X(x), Y(y), Z(z) {}

    Vector3& operator+=(const Vector3& other);

    float& operator[](int idx) { return *(&X + idx); }

    const float& operator[](int idx) const { return *(&X + idx); }

    float Length() const;
};

Vector3 operator+(const Vector3& a, const Vector3& b);
Vector3 operator-(const Vector3& a, const Vector3& b);
Vector3 operator*(float multiplier, const Vector3& vector);
Vector3 operator*(const Vector3& vector, float multiplier);
std::ostream& operator<<(std::ostream&, const Vector3&);
std::istream& operator>>(std::istream&, Vector3&);
bool operator==(const Vector3& a, const Vector3& b);

Vector3 takeMinimum(const Vector3& a, const Vector3& b);
Vector3 takeMaximum(const Vector3& a, const Vector3& b);

typedef Vector3 Vertex;
} // namespace math