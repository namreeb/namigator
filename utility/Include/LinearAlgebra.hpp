#pragma once

#include <iostream>
#include <fstream>

#ifndef PI
#define PI 3.14159264f
#endif

namespace utility
{
    // XXX maybe we don't really need quaternions \o/
    struct Quaternion
    {
        float W;
        float X;
        float Y;
        float Z;

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

    template <typename T>
    class Row
    {
        private:
            int Columns;
            T *row;

        public:
            Row(int columns = 0);
            Row(const Row &r);
            ~Row() { delete[] row; }

            void SetColumnSize(const int columns);

            T& operator [](int column);
            Row& operator =(const Row &r);
    };

    struct Vector3;
    typedef Vector3 Vertex;

    class Matrix
    {
        private:
            int Rows;
            int Columns;
            Row<float> *matrix;

        public:
            Matrix(int rows = 0, int columns = 0);

            // printing
            void Print(std::ostream &s = std::cout) const
            {
                s << Rows << " x " << Columns << std::endl;

                for (int r = 0; r < Rows; ++r)
                {
                    for (int c = 0; c < Columns; ++c)
                        s << matrix[r][c] << " ";

                    s << std::endl;
                }
            }

            Row<float>& operator [](int index) const;

            static Matrix CreateRotationX(float radians);
            static Matrix CreateRotationY(float radians);
            static Matrix CreateRotationZ(float radians);
            static Matrix CreateScalingMatrix(float scale);
            static Matrix CreateFromQuaternion(const Quaternion &quaternion);
            static Matrix CreateTranslationMatrix(const Vertex &position);
            static Matrix CreateViewMatrix(const Vertex &eye, const Vertex &target, const Vector3 &up);
            static Matrix CreateProjectionMatrix(float fovy, float aspect, float zNear, float zFar);
            static Matrix Transpose(const Matrix &in);

            friend Matrix operator * (const Matrix &a, const Matrix &b);

            void PopulateArray(float *out) const
            {
                for (int r = 0; r < Rows; ++r)
                    for (int c = 0; c < Columns; ++c)
                        out[r*Columns + c] = matrix[r][c];
            }
    };

    struct Vector2
    {
        const float X;
        const float Y;

        Vector2(float x, float y) : X(x), Y(y) {}
    };

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

        float Length() const;
    };

    Vector3 operator + (const Vector3 &a, const Vector3 &b);
    Vector3 operator - (const Vector3 &a, const Vector3 &b);
}