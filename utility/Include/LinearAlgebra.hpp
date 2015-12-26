#pragma once

#include <iostream>
#include <fstream>

#ifndef PI
#define PI 3.14159264f
#endif

namespace utility
{
    class Quaternion
    {
        public:
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

    class Vector3;

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
            //Matrix& operator =(Matrix &m);

            static Matrix CreateRotationX(float radians);
            static Matrix CreateRotationY(float radians);
            static Matrix CreateRotationZ(float radians);
            static Matrix CreateScalingMatrix(float scale);
            static Matrix CreateFromQuaternion(const Quaternion &quaternion);
            static Matrix CreateTranslationMatrix(const Vector3 &position);
            static Matrix CreateViewMatrix(const Vector3 &eye, const Vector3 &target, const Vector3 &up);
            static Matrix CreateViewMatrixFromLookNormal(const Vector3 &eye, const Vector3 &directionNormal, const Vector3 &up);
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

    class Vector2
    {
        public:
            const float X;
            const float Y;

            Vector2(float x, float y) : X(x), Y(y) {}
    };

    class Vector3
    {
        public:
            static float DotProduct(const Vector3 &a, const Vector3 &b)
            {
                return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
            }

            static Vector3 CrossProduct(const Vector3 &a, const Vector3 &b)
            {
                return Vector3(a.Y * b.Z - a.Z * b.Y, a.Z * b.X - a.X * b.Z, a.X * b.Y - a.Y * b.X);
            }

            static Vector3 Normalize(const Vector3 &a)
            {
                const float d = 1.f / sqrt(a.X * a.X + a.Y * a.Y + a.Z * a.Z);
                return Vector3(a.X * d, a.Y * d, a.Z * d);
            }

            static Vector3 Transform(float *position, Matrix matrix)
            {
                return Transform(*reinterpret_cast<Vector3 *>(position), matrix);
            }

            static Vector3 Transform(const Vector3 &position, const Matrix &matrix)
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

            float X;
            float Y;
            float Z;

            Vector3() : X(0.f), Y(0.f), Z(0.f) {}
            Vector3(float x, float y, float z) : X(x), Y(y), Z(z) {}
    };

    Vector3 operator + (const Vector3 &a, const Vector3 &b);
    Vector3 operator - (const Vector3 &a, const Vector3 &b);

    typedef Vector3 Vertex;
}