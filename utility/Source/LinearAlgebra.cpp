#include "utility/Include/LinearAlgebra.hpp"
#include "utility/Include/Exception.hpp"

#include <math.h>
#include <ostream>

namespace utility
{
Quaternion::Quaternion(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w) {}

Quaternion operator * (const Quaternion &a, const Quaternion &b)
{
    const float w = a.W*b.W - (a.X*b.X + a.Y*b.Y + a.Z*b.Z);

    const float x = a.W*b.X + b.W*a.X + a.Y*b.Z - a.Z*b.Y;
    const float y = a.W*b.Y + b.W*a.Y + a.Z*b.X - a.X*b.Z;
    const float z = a.W*b.Z + b.W*a.Z + a.X*b.Y - a.Y*b.X;

    return Quaternion(w, x, y, z);
}

const Quaternion& Quaternion::operator *= (const Quaternion &q)
{
    const float w = W*q.W - (X*q.X + Y*q.Y + Z*q.Z);

    const float x = W*q.X + q.W*X + Y*q.Z - Z*q.Y;
    const float y = W*q.Y + q.W*Y + Z*q.X - X*q.Z;
    const float z = W*q.Z + q.W*Z + X*q.Y - Y*q.X;

    W = w;
    X = x;
    Y = y;
    Z = z;
    return *this;
}

const Quaternion& Quaternion::operator ~ ()
{
    X = -X;
    Y = -Y;
    Z = -Z;
    return *this;
}

const Quaternion& Quaternion::operator - ()
{
    float norme = sqrtf(W*W + X*X + Y*Y + Z*Z);
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
    const float norme = sqrt(W*W + X*X + Y*Y + Z*Z);

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

void Quaternion::Print(std::ostream &s)
{
    s << "X: " << X << " Y: " << Y << " Z: " << Z << " W: " << W << std::endl;
}

Matrix::Matrix(int rows, int columns) : m_rows(rows), m_columns(columns), m_matrix(rows*columns) {}

float *Matrix::operator [](int row)
{
    if (row >= m_rows)
        THROW("Bad row");

    return &m_matrix[m_columns*row];
}

const float *Matrix::operator [](int row) const
{
    if (row >= m_rows)
        THROW("Bad row");

    return &m_matrix[m_columns*row];
}

// taken from: https://github.com/radekp/qt/blob/master/src/gui/math3d/qmatrix4x4.cpp#L1066
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
Matrix Matrix::CreateFromQuaternion(const Quaternion &q)
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
Matrix Matrix::CreateTranslationMatrix(const utility::Vertex &position)
{
    Matrix ret(4, 4);

    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            ret[y][x] = 0.f;

    ret[0][0] = ret[1][1] = ret[2][2] = ret[3][3] = 1.f;
    ret[3][0] = position.X;
    ret[3][1] = position.Y;
    ret[3][2] = position.Z;

    return ret;
}

// Possibly wrong, this is neither left nor right handed
Matrix Matrix::CreateViewMatrix(const utility::Vertex &eye, const utility::Vertex &target, const Vector3 &up)
{
    const Vector3 v_z = Vector3::Normalize(eye - target);
    const Vector3 v_x = Vector3::Normalize(Vector3::CrossProduct(up, v_z));
    const Vector3 v_y = Vector3::CrossProduct(v_z, v_x);

    const float xDotEye = Vector3::DotProduct(v_x, eye);
    const float yDotEye = Vector3::DotProduct(v_y, eye);
    const float zDotEye = Vector3::DotProduct(v_z, eye);

    Matrix ret(4, 4);

    ret[0][0] = v_x.X;    ret[0][1] = v_y.X;    ret[0][2] = v_z.X;    ret[0][3] = 0.f;
    ret[1][0] = v_x.Y;    ret[1][1] = v_y.Y;    ret[1][2] = v_z.Y;    ret[1][3] = 0.f;
    ret[2][0] = v_x.Z;    ret[2][1] = v_y.Z;    ret[2][2] = v_z.Z;    ret[2][3] = 0.f;
    ret[3][0] = -xDotEye; ret[3][1] = -yDotEye; ret[3][2] = -zDotEye; ret[3][3] = 1.f;

    return ret;
}

// Right handed
Matrix Matrix::CreateProjectionMatrix(float fovy, float aspect, float zNear, float zFar)
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
    ret[3][2] = zNear*zFar / (zNear - zFar);

    return ret;
}

Matrix operator *(const Matrix &a, const Matrix &b)
{
    if (a.m_columns != b.m_rows)
        THROW("Invalid matrix multiplication");

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

float Vector3::DotProduct(const Vector3 &a, const Vector3 &b)
{
    return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
}

Vector3 Vector3::CrossProduct(const Vector3 &a, const Vector3 &b)
{
    return Vector3(a.Y * b.Z - a.Z * b.Y, a.Z * b.X - a.X * b.Z, a.X * b.Y - a.Y * b.X);
}

Vector3 Vector3::Normalize(const Vector3 &a)
{
    const float d = 1.f / sqrt(a.X * a.X + a.Y * a.Y + a.Z * a.Z);
    return Vector3(a.X * d, a.Y * d, a.Z * d);
}

Vector3 Vector3::Transform(const Vector3 &position, const Matrix &matrix)
{
    Matrix vertexVector(4, 1);

    vertexVector[0][0] = position.X;
    vertexVector[1][0] = position.Y;
    vertexVector[2][0] = position.Z;
    vertexVector[3][0] = 1.f;

    // multiply matrix by column std::vector matrix
    const Matrix newVector = matrix * vertexVector;

    return Vector3(newVector[0][0], newVector[1][0], newVector[2][0]);
}

Vector3 &Vector3::operator +=(const Vector3 & other)
{
    X += other.X;
    Y += other.Y;
    Z += other.Z;

    return *this;
}

float Vector3::Length() const
{
    return sqrtf(X*X + Y*Y + Z*Z);
}

Vector3 operator + (const Vector3 &a, const Vector3 &b)
{
    return Vector3(a.X + b.X, a.Y + b.Y, a.Z + b.Z);
}

Vector3 operator - (const Vector3 &a, const Vector3 &b)
{
    return Vector3(a.X - b.X, a.Y - b.Y, a.Z - b.Z);
}

Vector3 operator * (float multiplier, const Vector3 &vector)
{
    return Vector3(vector.X*multiplier, vector.Y*multiplier, vector.Z*multiplier);
}

Vector3 operator * (const Vector3 &vector, float multiplier)
{
    return Vector3(vector.X*multiplier, vector.Y*multiplier, vector.Z*multiplier);
}
}