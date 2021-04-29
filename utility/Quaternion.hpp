#pragma once

#include <iostream>

namespace math
{
struct Quaternion
{
    float X;
    float Y;
    float Z;
    float W;

    Quaternion(float x = 0.0, float y = 0.0, float z = 0.0, float w = 0.0);

    // printing
    void Print(std::ostream& = std::cout);

    // quaternion multiplication
    friend Quaternion operator*(const Quaternion& a, const Quaternion& b);
    const Quaternion& operator*=(const Quaternion& a);

    // conjugate
    const Quaternion& operator~();

    // invert
    const Quaternion& operator-();

    // normalize
    const Quaternion& Normalize();
};
} // namespace math
