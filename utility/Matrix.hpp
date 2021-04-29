#pragma once

#include "utility/BinaryStream.hpp"
#include "utility/Vector.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <ostream>
#include <vector>

#ifndef PI
#    define PI 3.14159264f
#endif

namespace math
{
class BinaryStream;
struct Quaternion;

class Matrix
{
private:
    int m_rows;
    int m_columns;
    std::vector<float> m_matrix;

public:
    Matrix(int rows = 0, int columns = 0);

    // printing
    void Print(std::ostream& s = std::cout) const
    {
        s << m_rows << " x " << m_columns << std::endl;

        for (int r = 0; r < m_rows; ++r)
        {
            for (int c = 0; c < m_columns; ++c)
                s << m_matrix[r * m_columns + c] << " ";

            s << std::endl;
        }
    }

    float* operator[](int row);
    const float* operator[](int row) const;

    static Matrix CreateRotationX(float radians)
    {
        return CreateRotation(Vector3(1.f, 0.f, 0.f), radians);
    }
    static Matrix CreateRotationY(float radians)
    {
        return CreateRotation(Vector3(0.f, 1.f, 0.f), radians);
    }
    static Matrix CreateRotationZ(float radians)
    {
        return CreateRotation(Vector3(0.f, 0.f, 1.f), radians);
    }
    static Matrix CreateRotation(Vector3 direction, float radians);
    static Matrix CreateScalingMatrix(float scale);
    static Matrix CreateFromQuaternion(const Quaternion& quaternion);
    static Matrix CreateTranslationMatrix(const math::Vector3& position);
    static Matrix CreateViewMatrix(const math::Vector3& eye,
                                   const math::Vector3& target,
                                   const Vector3& up);
    static Matrix CreateProjectionMatrix(float fovy, float aspect, float zNear,
                                         float zFar);
    static Matrix CreateFromArray(const float* in, int count);

    Matrix Transposed() const;

    float ComputeDeterminant() const;
    Matrix ComputeInverse() const;

    friend Matrix operator*(const Matrix& a, const Matrix& b);
    friend utility::BinaryStream& operator<<(utility::BinaryStream&,
                                             const Matrix&);

    void PopulateArray(float* out) const
    {
        ::memcpy(out, &m_matrix[0], m_matrix.size() * sizeof(float));
    }
};

utility::BinaryStream& operator<<(utility::BinaryStream&, const Matrix&);
} // namespace math