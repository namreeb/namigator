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

    template <typename T> class Vector3;

    template <typename T>
    class Matrix
    {
        private:
            int Rows;
            int Columns;
            Row<T> *matrix;

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

            Row<T>& operator [](int index) const;
            Matrix<T>& operator =(Matrix<T> &m);

            static Matrix<float> CreateRotationX(float radians);
            static Matrix<float> CreateRotationY(float radians);
            static Matrix<float> CreateRotationZ(float radians);
            static Matrix<float> CreateScalingMatrix(float scale);
            static Matrix<float> CreateFromQuaternion(const Quaternion &quaternion);
            static Matrix<float> CreateTranslationMatrix(const Vector3<float> &position);
            static Matrix<float> CreateViewMatrix(const Vector3<float> &eye, const Vector3<float> &target, const Vector3<float> &up);
            static Matrix<float> CreateProjectionMatrix(float fovy, float aspect, float zNear, float zFar);
            static Matrix<float> Transpose(const Matrix<float> &in);

            template <typename T>
            friend Matrix<T> operator * (const Matrix<T> &a, const Matrix<T> &b);

            void PopulateArray(T *out) const
            {
                for (int r = 0; r < Rows; ++r)
                    for (int c = 0; c < Columns; ++c)
                        out[r*Columns + c] = matrix[r][c];
            }
    };

    template <typename T>
    Matrix<T> operator *(const Matrix<T> &a, const Matrix<T> &b)
    {
        if (a.Columns != b.Rows)
            throw "Invalid matrix multiplication";

        Matrix<T> ret(a.Rows, b.Columns);

        for (int r = 0; r < a.Rows; ++r)
            for (int c = 0; c < b.Columns; ++c)
            {
                ret[r][c] = (T)0;

                for (int i = 0; i < a.Columns; ++i)
                    ret[r][c] += a.matrix[r][i] * b[i][c];
            }

        return ret;
    }

    template <typename T>
    class Vector2
    {
        public:
            T X;
            T Y;

            Vector2() : X((T)0), Y((T)0) {}
            Vector2(T x, T y) : X(x), Y(y) {}

            static inline T AreaSquared(const Vector2<T> &a, const Vector2<T> &b, const Vector2<T> &c)
            {
                return (b.X - a.X) * (c.Y - a.Y) - (c.X - a.X) * (b.Y - a.Y);
            }

            static inline bool IsLeft(const Vector2<T> &a, const Vector2<T> &b, const Vector2<T> &c)
            {
                return AreaSquared(a, b, c) < 0;
            }

            static void SaveToDisk(const Vector2<T> &a, const Vector2<T> &b, const Vector2<T> &c)
            {
                int minX, minY, maxX, maxY;

                minX = maxX = a.X;
                minY = maxY = a.Y;

                if (b.X > maxX)
                    maxX = b.X;
                if (b.X < minX)
                    minX = b.X;
                if (b.Y > maxY)
                    maxY = b.Y;
                if (b.Y < minY)
                    minY = b.Y;
                if (c.X > maxX)
                    maxX = c.X;
                if (c.X < minX)
                    minX = c.X;
                if (c.Y > maxY)
                    maxY = c.Y;
                if (c.Y < minY)
                    minY = c.Y;

                std::ofstream out("d:\\poly.gnuplot");
                std::ofstream data("d:\\poly.dat");

                out << "reset" << std::endl;
                out << "set term windows" << std::endl;
                out << "unset key" << std::endl;
                out << "set xrange [" << (minY - 5) << ":" << (maxY + 5) << "] reverse" << std::endl;
                out << "set xlabel 'Y'" << std::endl;
                out << "set yrange [" << (minX - 5) << ":" << (maxX + 5) << "]" << std::endl;
                out << "set ylabel 'X'" << std::endl;

                out << "set label \"0: (" << a.Y << ", " << a.X << ")\" at " << a.Y + 1 << ", " << a.X + 1 << std::endl;
                out << "set label \"1: (" << b.Y << ", " << b.X << ")\" at " << b.Y + 1 << ", " << b.X + 1 << std::endl;
                out << "set label \"2: (" << c.Y << ", " << c.X << ")\" at " << c.Y + 1 << ", " << c.X + 1 << std::endl;

                data << a.Y << " " << a.X << std::endl;
                data << b.Y << " " << b.X << std::endl;
                data << c.Y << " " << c.X << std::endl;
                data << a.Y << " " << a.X << std::endl;
            }
    };

    template <typename T>
    class Vector3 : public Vector2<T>
    {
        public:
            T Z;

            Vector3<T>() : Vector2<T>(), Z((T)0) {}
            Vector3<T>(T x, T y, T z) : Vector2<T>(x, y), Z(z) {}

            static T DotProduct(const Vector3<T> &a, const Vector3<T> &b)
            {
                return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
            }

            static Vector3<T> CrossProduct(const Vector3<T> &a, const Vector3<T> &b)
            {
                return Vector3<T>(a.Y * b.Z - a.Z * b.Y, a.Z * b.X - a.X * b.Z, a.X * b.Y - a.Y * b.X);
            }

            static Vector3<T> Normalize(const Vector3<T> &a)
            {
                const float d = 1.f / sqrt(a.X * a.X + a.Y * a.Y + a.Z * a.Z);
                return Vector3<T>(a.X * d, a.Y * d, a.Z * d);
            }    

            static Vector3<T> Transform(T *position, Matrix<T> matrix)
            {
                return Transform(*(Vertex *)position, matrix);
            }

            static Vector3<T> Transform(const Vector3<T> &position, const Matrix<T> &matrix)
            {
                Matrix<T> vertexVector(4, 1);

                vertexVector[0][0] = position.X;
                vertexVector[1][0] = position.Y;
                vertexVector[2][0] = position.Z;
                vertexVector[3][0] = 1.f;

                // multiply matrix by column std::vector matrix
                Matrix<T> newVector = matrix * (const Matrix<T>)vertexVector;

                return Vertex(newVector[0][0], newVector[1][0], newVector[2][0]);
            }
    };

    typedef Vector3<float> Vertex;

    template <typename T>
    Vector3<T> operator + (const Vector3<T> &a, const Vector3<T> &b)
    {
        return Vector3<T>(a.X + b.X, a.Y + b.Y, a.Z + b.Z);
    }

    template <typename T>
    Vector3<T> operator - (const Vector3<T> &a, const Vector3<T> &b)
    {
        return Vector3<T>(a.X - b.X, a.Y - b.Y, a.Z - b.Z);
    }
}