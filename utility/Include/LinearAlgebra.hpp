#pragma once

#include <iostream>
#include <vector>
#include <algorithm>
#include <ostream>

#ifndef PI
#define PI 3.14159264f
#endif

namespace utility
{
struct Quaternion
{
    float X;
    float Y;
    float Z;
    float W;

    Quaternion(float x = 0.0, float y = 0.0, float z = 0.0, float w = 0.0);

    // printing
    void Print(std::ostream & = std::cout);

    // quaternion multiplication
    friend Quaternion operator * (const Quaternion &a, const Quaternion &b);
    const Quaternion& operator *= (const Quaternion &a);

    // conjugate
    const Quaternion& operator ~();

    // invert
    const Quaternion& operator -();

    // normalize
    const Quaternion& Normalize();
};

struct Vector2
{
    const float X;
    const float Y;

    Vector2(float x, float y) : X(x), Y(y) {}
};

class Matrix;

struct Vector3
{
    static float DotProduct(const Vector3 &a, const Vector3 &b);
    static Vector3 CrossProduct(const Vector3 &a, const Vector3 &b);
    static Vector3 Normalize(const Vector3 &a);
    static Vector3 Transform(const Vector3 &position, const Matrix &matrix);

    float X;
    float Y;
    float Z;

    Vector3() : X(0.f), Y(0.f), Z(0.f) {}
    Vector3(float x, float y, float z) : X(x), Y(y), Z(z) {}

    Vector3& operator += (const Vector3 &other);

    float& operator[](int idx) {
        return *(&X + idx);
    }

    const float& operator[](int idx) const {
        return *(&X + idx);
    }

    float Length() const;
};

Vector3 operator + (const Vector3 &a, const Vector3 &b);
Vector3 operator - (const Vector3 &a, const Vector3 &b);
Vector3 operator * (float multiplier, const Vector3 &vector);
Vector3 operator * (const Vector3& vector, float multiplier);
std::ostream & operator << (std::ostream &, const Vector3 &);
std::istream & operator >> (std::istream &, Vector3 &);

typedef Vector3 Vertex;

class Matrix
{
    private:
        int m_rows;
        int m_columns;
        std::vector<float> m_matrix;

    public:
        Matrix(int rows = 0, int columns = 0);

        // printing
        void Print(std::ostream &s = std::cout) const
        {
            s << m_rows << " x " << m_columns << std::endl;

            for (int r = 0; r < m_rows; ++r)
            {
                for (int c = 0; c < m_columns; ++c)
                    s << m_matrix[r*m_columns + c] << " ";

                s << std::endl;
            }
        }

        float *operator [](int row);
        const float *operator [](int row) const;

        static Matrix CreateRotationX(float radians) { return CreateRotation(Vector3(1.f, 0.f, 0.f), radians); }
        static Matrix CreateRotationY(float radians) { return CreateRotation(Vector3(0.f, 1.f, 0.f), radians); }
        static Matrix CreateRotationZ(float radians) { return CreateRotation(Vector3(0.f, 0.f, 1.f), radians); }
        static Matrix CreateRotation(Vector3 direction, float radians);
        static Matrix CreateScalingMatrix(float scale);
        static Matrix CreateFromQuaternion(const Quaternion &quaternion);
        static Matrix CreateTranslationMatrix(const utility::Vertex &position);
        static Matrix CreateViewMatrix(const utility::Vertex &eye, const utility::Vertex &target, const Vector3 &up);
        static Matrix CreateProjectionMatrix(float fovy, float aspect, float zNear, float zFar);

        float ComputeDeterminant() const;
        Matrix ComputeInverse() const;

        friend Matrix operator * (const Matrix &a, const Matrix &b);

        void PopulateArray(float *out) const
        {
            memcpy(out, &m_matrix[0], m_matrix.size()*sizeof(float));
        }
};

template <typename Vector>
Vector takeMinimum(const Vector& a, const Vector& b) {
    return Vector{
        std::min(a.X, b.X),
        std::min(a.Y, b.Y),
        std::min(a.Z, b.Z)
    };
}

template <typename Vector>
Vector takeMaximum(const Vector& a, const Vector& b) {
    return Vector{
        std::max(a.X, b.X),
        std::max(a.Y, b.Y),
        std::max(a.Z, b.Z)
    };
}
}