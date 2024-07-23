#include "utility/Vector.hpp"

#include "utility/Matrix.hpp"

#include <cmath>

namespace math
{
float Vector3::DotProduct(const Vector3& a, const Vector3& b)
{
    return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
}

Vector3 Vector3::CrossProduct(const Vector3& a, const Vector3& b)
{
    return Vector3(a.Y * b.Z - a.Z * b.Y, a.Z * b.X - a.X * b.Z,
                   a.X * b.Y - a.Y * b.X);
}

float Vector3::GetDistance(const Vector3& other) const
{
    return sqrtf(powf((X - other.X), 2) + powf((Y - other.Y), 2) +
                 powf((Z - other.Z), 2));
}

Vector3 Vector3::Normalize(const Vector3& a)
{
    const float d = 1.f / sqrt(a.X * a.X + a.Y * a.Y + a.Z * a.Z);
    return Vector3(a.X * d, a.Y * d, a.Z * d);
}

Vector3 Vector3::Transform(const Vector3& position, const Matrix& matrix)
{
    Matrix Vector3Vector(4, 1);

    Vector3Vector[0][0] = position.X;
    Vector3Vector[1][0] = position.Y;
    Vector3Vector[2][0] = position.Z;
    Vector3Vector[3][0] = 1.0f;

    // multiply matrix by column std::vector matrix
    const Matrix newVector = matrix * Vector3Vector;

    float w = 1.0f / newVector[3][0];
    return Vector3(newVector[0][0] * w, newVector[1][0] * w,
                   newVector[2][0] * w);
}

Vector3& Vector3::operator+=(const Vector3& other)
{
    X += other.X;
    Y += other.Y;
    Z += other.Z;

    return *this;
}

float Vector3::Length() const
{
    return sqrtf(X * X + Y * Y + Z * Z);
}

Vector3 operator+(const Vector3& a, const Vector3& b)
{
    return Vector3(a.X + b.X, a.Y + b.Y, a.Z + b.Z);
}

Vector3 operator-(const Vector3& a, const Vector3& b)
{
    return Vector3(a.X - b.X, a.Y - b.Y, a.Z - b.Z);
}

Vector3 operator*(float multiplier, const Vector3& vector)
{
    return Vector3(vector.X * multiplier, vector.Y * multiplier,
                   vector.Z * multiplier);
}

Vector3 operator*(const Vector3& vector, float multiplier)
{
    return Vector3(vector.X * multiplier, vector.Y * multiplier,
                   vector.Z * multiplier);
}

std::ostream& operator<<(std::ostream& o, const Vector3& v)
{
    o.write(reinterpret_cast<const char*>(&v), sizeof(v));
    return o;
}

std::istream& operator>>(std::istream& i, Vector3& v)
{
    i.read(reinterpret_cast<char*>(&v), sizeof(v));
    return i;
}

bool operator==(const Vector3& a, const Vector3& b)
{
    return a.X == b.X && a.Y == b.Y && a.Z == b.Z;
}

Vector3 takeMinimum(const Vector3& a, const Vector3& b)
{
    return Vector3 {std::min(a.X, b.X), std::min(a.Y, b.Y), std::min(a.Z, b.Z)};
}

Vector3 takeMaximum(const Vector3& a, const Vector3& b)
{
    return Vector3 {std::max(a.X, b.X), std::max(a.Y, b.Y), std::max(a.Z, b.Z)};
}
} // namespace math
