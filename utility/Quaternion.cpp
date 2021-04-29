#include "utility/Quaternion.hpp"

#include <cmath>

namespace math
{
Quaternion::Quaternion(float x, float y, float z, float w)
    : X(x), Y(y), Z(z), W(w)
{
}

Quaternion operator*(const Quaternion& a, const Quaternion& b)
{
    const float w = a.W * b.W - (a.X * b.X + a.Y * b.Y + a.Z * b.Z);

    const float x = a.W * b.X + b.W * a.X + a.Y * b.Z - a.Z * b.Y;
    const float y = a.W * b.Y + b.W * a.Y + a.Z * b.X - a.X * b.Z;
    const float z = a.W * b.Z + b.W * a.Z + a.X * b.Y - a.Y * b.X;

    return Quaternion(w, x, y, z);
}

const Quaternion& Quaternion::operator*=(const Quaternion& q)
{
    const float w = W * q.W - (X * q.X + Y * q.Y + Z * q.Z);

    const float x = W * q.X + q.W * X + Y * q.Z - Z * q.Y;
    const float y = W * q.Y + q.W * Y + Z * q.X - X * q.Z;
    const float z = W * q.Z + q.W * Z + X * q.Y - Y * q.X;

    W = w;
    X = x;
    Y = y;
    Z = z;
    return *this;
}

const Quaternion& Quaternion::operator~()
{
    X = -X;
    Y = -Y;
    Z = -Z;
    return *this;
}

const Quaternion& Quaternion::operator-()
{
    float norme = sqrtf(W * W + X * X + Y * Y + Z * Z);
    if (norme == 0.0)
        norme = 1.0;

    const float recip = 1.f / norme;

    W = W * recip;
    X = -X * recip;
    Y = -Y * recip;
    Z = -Z * recip;

    return *this;
}

const Quaternion& Quaternion::Normalize()
{
    const float norme = sqrt(W * W + X * X + Y * Y + Z * Z);

    if (norme == 0.0)
    {
        W = 1.f;
        X = Y = Z = 0.f;
    }
    else
    {
        const float recip = 1.f / norme;

        W *= recip;
        X *= recip;
        Y *= recip;
        Z *= recip;
    }

    return *this;
}

void Quaternion::Print(std::ostream& s)
{
    s << "X: " << X << " Y: " << Y << " Z: " << Z << " W: " << W << std::endl;
}
} // namespace math
