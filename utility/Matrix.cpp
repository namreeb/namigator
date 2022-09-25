#include "utility/Matrix.hpp"

#include "utility/BinaryStream.hpp"
#include "utility/Exception.hpp"
#include "utility/Quaternion.hpp"

#include <cassert>
#include <cmath>
#include <ostream>

namespace math
{
Matrix::Matrix(int rows, int columns)
    : m_rows(rows), m_columns(columns), m_matrix(rows * columns)
{
}

float* Matrix::operator[](int row)
{
    if (row >= m_rows)
        THROW(Result::BAD_MATRIX_ROW);

    return &m_matrix[m_columns * row];
}

const float* Matrix::operator[](int row) const
{
    if (row >= m_rows)
        THROW(Result::BAD_MATRIX_ROW);

    return &m_matrix[m_columns * row];
}

// taken from:
// https://github.com/radekp/qt/blob/master/src/gui/math3d/qmatrix4x4.cpp#L1066
Matrix Matrix::CreateRotation(Vector3 direction, float radians)
{
    Matrix ret(4, 4);

    const float c = cosf(radians);
    const float ic = 1.f - c;
    const float s = sinf(radians);

    ret[0][3] = ret[1][3] = ret[2][3] = ret[3][0] = ret[3][1] = ret[3][2] = 0.f;
    ret[3][3] = 1.f;
    ret[0][0] = direction.X * direction.X * ic + c;
    ret[0][1] = direction.X * direction.Y * ic - direction.Z * s;
    ret[0][2] = direction.X * direction.Z * ic + direction.Y * s;
    ret[1][0] = direction.Y * direction.X * ic + direction.Z * s;
    ret[1][1] = direction.Y * direction.Y * ic + c;
    ret[1][2] = direction.Y * direction.Z * ic - direction.X * s;
    ret[2][0] = direction.X * direction.Z * ic - direction.Y * s;
    ret[2][1] = direction.Y * direction.Z * ic + direction.X * s;
    ret[2][2] = direction.Z * direction.Z * ic + c;

    return ret;
}

Matrix Matrix::CreateScalingMatrix(float scale)
{
    Matrix ret(4, 4);

    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            ret[y][x] = 0.f;

    ret[0][0] = ret[1][1] = ret[2][2] = scale;
    ret[3][3] = 1.f;

    return ret;
}

// taken from: http://www.flipcode.com/documents/matrfaq.html#Q54
Matrix Matrix::CreateFromQuaternion(const Quaternion& q)
{
    Matrix ret(4, 4);

    const float xx = q.X * q.X;
    const float xy = q.X * q.Y;
    const float xz = q.X * q.Z;
    const float xw = q.X * q.W;

    const float yy = q.Y * q.Y;
    const float yz = q.Y * q.Z;
    const float yw = q.Y * q.W;

    const float zz = q.Z * q.Z;
    const float zw = q.Z * q.W;

    ret[0][0] = (float)(1.0 - 2.0 * (yy + zz));
    ret[0][1] = (float)(2.0 * (xy - zw));
    ret[0][2] = (float)(2.0 * (xz + yw));

    ret[1][0] = (float)(2.0 * (xy + zw));
    ret[1][1] = (float)(1.0 - 2.0 * (xx + zz));
    ret[1][2] = (float)(2.0 * (yz - xw));

    ret[2][0] = (float)(2.0 * (xz - yw));
    ret[2][1] = (float)(2.0 * (yz + xw));
    ret[2][2] = (float)(1.0 - 2.0 * (xx + yy));

    ret[0][3] = ret[1][3] = ret[2][3] = ret[3][0] = ret[3][1] = ret[3][2] = 0.f;
    ret[3][3] = 1.f;

    return ret;
}

// taken from MaiN's XNA Math lib
Matrix Matrix::CreateTranslationMatrix(const math::Vector3& position)
{
    Matrix ret(4, 4);

    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            ret[y][x] = 0.f;

    ret[0][0] = ret[1][1] = ret[2][2] = ret[3][3] = 1.f;
    ret[0][3] = position.X;
    ret[1][3] = position.Y;
    ret[2][3] = position.Z;

    return ret;
}

// Possibly wrong, this is neither left nor right handed
Matrix Matrix::CreateViewMatrix(const math::Vector3& eye,
                                const math::Vector3& target, const Vector3& up)
{
    const Vector3 v_z = Vector3::Normalize(eye - target);
    const Vector3 v_x = Vector3::Normalize(Vector3::CrossProduct(up, v_z));
    const Vector3 v_y = Vector3::CrossProduct(v_z, v_x);

    const float xDotEye = Vector3::DotProduct(v_x, eye);
    const float yDotEye = Vector3::DotProduct(v_y, eye);
    const float zDotEye = Vector3::DotProduct(v_z, eye);

    Matrix ret(4, 4);

    ret[0][0] = v_x.X;
    ret[0][1] = v_y.X;
    ret[0][2] = v_z.X;
    ret[0][3] = 0.f;
    ret[1][0] = v_x.Y;
    ret[1][1] = v_y.Y;
    ret[1][2] = v_z.Y;
    ret[1][3] = 0.f;
    ret[2][0] = v_x.Z;
    ret[2][1] = v_y.Z;
    ret[2][2] = v_z.Z;
    ret[2][3] = 0.f;
    ret[3][0] = -xDotEye;
    ret[3][1] = -yDotEye;
    ret[3][2] = -zDotEye;
    ret[3][3] = 1.f;

    return ret;
}

// Right handed
Matrix Matrix::CreateProjectionMatrix(float fovy, float aspect, float zNear,
                                      float zFar)
{
    Matrix ret(4, 4);

    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            ret[y][x] = 0.f;

    const float yscale = 1.f / tanf(0.5f * fovy);
    const float xscale = yscale / aspect;

    ret[0][0] = xscale;
    ret[1][1] = yscale;
    ret[2][2] = zFar / (zNear - zFar);
    ret[2][3] = -1.f;
    ret[3][2] = zNear * zFar / (zNear - zFar);

    return ret;
}

Matrix Matrix::CreateFromArray(const float* in, int count)
{
    const int len = static_cast<int>(sqrt(count));
    assert(len * len == count);

    Matrix ret(len, len);

    memcpy(&ret.m_matrix[0], in, sizeof(float) * count);

    return ret;
}

Matrix operator*(const Matrix& a, const Matrix& b)
{
    if (a.m_columns != b.m_rows)
        THROW(Result::INVALID_MATRIX_MULTIPLICATION);

    Matrix ret(a.m_rows, b.m_columns);

    for (int r = 0; r < a.m_rows; ++r)
        for (int c = 0; c < b.m_columns; ++c)
        {
            ret[r][c] = 0.f;

            for (int i = 0; i < a.m_columns; ++i)
                ret[r][c] += a[r][i] * b[i][c];
        }

    return ret;
}

Matrix Matrix::Transposed() const
{
    const Matrix& m = *this;
    Matrix ret(m_rows, m_columns);

    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_columns; ++c)
            ret[r][c] = m[c][r];
    return ret;
}

namespace
{
float Determinant3x3(const Matrix& m, int col0, int col1, int col2, int row0,
                     int row1, int row2)
{
    return m[row0][col0] *
               (m[row1][col1] * m[row2][col2] - m[row2][col1] * m[row1][col2]) -
           m[row0][col1] *
               (m[row1][col0] * m[row2][col2] - m[row2][col0] * m[row1][col2]) +
           m[row0][col2] *
               (m[row1][col0] * m[row2][col1] - m[row2][col0] * m[row1][col1]);
}
} // namespace

float Matrix::ComputeDeterminant() const
{
    if (m_columns != 4 || m_rows != 4)
        THROW(Result::ONLY_4X4_MATRIX_IS_SUPPORTED);

    const Matrix& m = *this;

    float det;
    det = m[0][0] * Determinant3x3(m, 1, 2, 3, 1, 2, 3);
    det -= m[0][1] * Determinant3x3(m, 0, 2, 3, 1, 2, 3);
    det += m[0][2] * Determinant3x3(m, 0, 1, 3, 1, 2, 3);
    det -= m[0][3] * Determinant3x3(m, 0, 1, 2, 1, 2, 3);

    return det;
}

Matrix Matrix::ComputeInverse() const
{
    if (m_columns != 4 || m_rows != 4)
        THROW(Result::ONLY_4X4_MATRIX_IS_SUPPORTED);

    float det = ComputeDeterminant();
    if (fabs(det) < 9e-7f)
    {
        THROW(Result::MATRIX_NOT_INVERTIBLE);
    }

    det = 1.0f / det;
    const Matrix& m = *this;

    Matrix inv(4, 4);
    inv[0][0] = Determinant3x3(m, 1, 2, 3, 1, 2, 3) * det;
    inv[1][0] = -Determinant3x3(m, 0, 2, 3, 1, 2, 3) * det;
    inv[2][0] = Determinant3x3(m, 0, 1, 3, 1, 2, 3) * det;
    inv[3][0] = -Determinant3x3(m, 0, 1, 2, 1, 2, 3) * det;
    inv[0][1] = -Determinant3x3(m, 1, 2, 3, 0, 2, 3) * det;
    inv[1][1] = Determinant3x3(m, 0, 2, 3, 0, 2, 3) * det;
    inv[2][1] = -Determinant3x3(m, 0, 1, 3, 0, 2, 3) * det;
    inv[3][1] = Determinant3x3(m, 0, 1, 2, 0, 2, 3) * det;
    inv[0][2] = Determinant3x3(m, 1, 2, 3, 0, 1, 3) * det;
    inv[1][2] = -Determinant3x3(m, 0, 2, 3, 0, 1, 3) * det;
    inv[2][2] = Determinant3x3(m, 0, 1, 3, 0, 1, 3) * det;
    inv[3][2] = -Determinant3x3(m, 0, 1, 2, 0, 1, 3) * det;
    inv[0][3] = -Determinant3x3(m, 1, 2, 3, 0, 1, 2) * det;
    inv[1][3] = Determinant3x3(m, 0, 2, 3, 0, 1, 2) * det;
    inv[2][3] = -Determinant3x3(m, 0, 1, 3, 0, 1, 2) * det;
    inv[3][3] = Determinant3x3(m, 0, 1, 2, 0, 1, 2) * det;
    return inv;
}

utility::BinaryStream& operator<<(utility::BinaryStream& o, const Matrix& m)
{
    o.Write(&m.m_matrix[0], sizeof(float) * m.m_matrix.size());
    return o;
}
} // namespace math