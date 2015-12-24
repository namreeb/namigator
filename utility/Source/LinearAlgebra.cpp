#include <math.h>
#include <ostream>
#include "LinearAlgebra.hpp"

namespace utility
{
    Quaternion::Quaternion(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w) {}

    Quaternion operator * (const Quaternion &a, const Quaternion &b)
    {
        float x, y, z, w;

        w = a.W*b.W - (a.X*b.X + a.Y*b.Y + a.Z*b.Z);

        x = a.W*b.X + b.W*a.X + a.Y*b.Z - a.Z*b.Y;
        y = a.W*b.Y + b.W*a.Y + a.Z*b.X - a.X*b.Z;
        z = a.W*b.Z + b.W*a.Z + a.X*b.Y - a.Y*b.X;

        return Quaternion(w,x,y,z);
    }

    const Quaternion& Quaternion::operator *= (const Quaternion &q)
    {
        float w = W*q.W - (X*q.X + Y*q.Y + Z*q.Z);

        float x = W*q.X + q.W*X + Y*q.Z - Z*q.Y;
        float y = W*q.Y + q.W*Y + Z*q.X - X*q.Z;
        float z = W*q.Z + q.W*Z + X*q.Y - Y*q.X;

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
        float norme = sqrt(W*W + X*X + Y*Y + Z*Z);
        if (norme == 0.0)
            norme = 1.0;

        const float recip = 1.f / norme;

        W =  W * recip;
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
            const float recip = 1.f/norme;

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

    template <typename T>
    Row<T>::Row(int columns)
    {
        Columns = columns;
        row = new T[Columns];
    }

    template <typename T>
    Row<T>::Row(const Row &r)
    {
        Columns = r.Columns;
        row = new T[Columns];

        for (int i = 0; i < Columns; ++i)
            row[i] = r[i];
    }

    template <typename T>
    void Row<T>::SetColumnSize(const int columns)
    {
        if (row)
            delete[] row;

        Columns = columns;

        row = new T[Columns];
    }

    template <typename T>
    T &Row<T>::operator [](int column)
    {
        if (column >= Columns)
            throw "Bad column index";

        return row[column];
    }

    template <typename T>
    Row<T> & Row<T>::operator =(const Row<T> &r)
    {
        if (row)
            delete[] row;

        Columns = r.Columns;

        row = new T[Columns];

        for (int i = 0; i < Columns; ++i)
            row[i] = r[i];

        return *this;
    }

    Matrix::Matrix(int rows, int columns)
    {
        Rows = rows;
        Columns = columns;

        matrix = (rows > 0 && columns > 0 ? new Row<float>[rows] : nullptr);

        for (int i = 0; i < rows; ++i)
            matrix[i].SetColumnSize(columns);
    }

    Row<float>& Matrix::operator [](int index) const
    {
        if (index >= Rows)
            throw "Bad row index";

        return matrix[index];
    }

    //Matrix& Matrix::operator =(Matrix &m)
    //{
    //    Rows = m.Rows;
    //    Columns = m.Columns;

    //    if (matrix)
    //        delete[] matrix;

    //    matrix = (M.Rows > 0 && M.columns > 0 ? new Row<float>[rows](columns) : nullptr);

    //    for (int r = 0; r < Rows; ++r)
    //        matrix[r] = Row<float>(m[r]);

    //    return *this;
    //}

    Matrix Matrix::CreateRotationX(float radians)
    {
        Matrix ret(4,4);

        ret[0][0] = 1.f;
        ret[0][1] = 0.f;
        ret[0][2] = 0.f;
        ret[1][0] = 0.f;
        ret[2][0] = 0.f;
        ret[3][0] = 0.f;
        ret[3][1] = 0.f;
        ret[3][2] = 0.f;
        ret[3][3] = 1.f;
        ret[0][3] = 0.f;
        ret[1][3] = 0.f;
        ret[2][3] = 0.f;

        ret[1][1] =  cosf(radians);
        ret[1][2] = -sinf(radians);
        ret[2][1] =  sinf(radians);
        ret[2][2] =  cosf(radians);

        return ret;
    }

    Matrix Matrix::CreateRotationY(float radians)
    {
        Matrix ret(4, 4);

        ret[1][1] = 1.f;
        ret[0][1] = 0.f;
        ret[1][2] = 0.f;
        ret[1][0] = 0.f;
        ret[2][1] = 0.f;
        ret[3][0] = 0.f;
        ret[3][1] = 0.f;
        ret[3][2] = 0.f;
        ret[3][3] = 1.f;
        ret[0][3] = 0.f;
        ret[1][3] = 0.f;
        ret[2][3] = 0.f;

        ret[0][0] =  cosf(radians);
        ret[0][2] =  sinf(radians);
        ret[2][0] = -sinf(radians);
        ret[2][2] =  cosf(radians);

        return ret;
    }

    Matrix Matrix::CreateRotationZ(float radians)
    {
        Matrix ret(4, 4);

        ret[2][2] = 1.f;
        ret[0][2] = 0.f;
        ret[1][2] = 0.f;
        ret[2][0] = 0.f;
        ret[2][1] = 0.f;
        ret[3][0] = 0.f;
        ret[3][1] = 0.f;
        ret[3][2] = 0.f;
        ret[3][3] = 1.f;
        ret[0][3] = 0.f;
        ret[1][3] = 0.f;
        ret[2][3] = 0.f;

        ret[0][0] =  cosf(radians);
        ret[0][1] = -sinf(radians);
        ret[1][0] =  sinf(radians);
        ret[1][1] =  cosf(radians);

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
    Matrix Matrix::CreateTranslationMatrix(const Vector3<float> &position)
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

    Matrix Matrix::CreateViewMatrix(const Vector3<float> &eye, const Vector3<float> &target, const Vector3<float> &up)
    {
        return CreateViewMatrixFromLookNormal(eye, Vertex::Normalize(eye - target), up);
    }

    Matrix Matrix::CreateViewMatrixFromLookNormal(const Vector3<float> &eye, const Vector3<float> &directionNormal, const Vector3<float> &up)
    {
        const Vertex v_z = Vertex::Normalize(directionNormal);
        const Vertex v_x = Vertex::Normalize(Vertex::CrossProduct(up, v_z));
        const Vertex v_y = Vertex::CrossProduct(v_z, v_x);

        const float xDotEye = Vertex::DotProduct(v_x, eye);
        const float yDotEye = Vertex::DotProduct(v_y, eye);
        const float zDotEye = Vertex::DotProduct(v_z, eye);

        Matrix ret(4, 4);

        ret[0][0] = v_x.X;    ret[0][1] = v_y.X;    ret[0][2] = v_z.X;    ret[0][3] = 0.f;
        ret[1][0] = v_x.Y;    ret[1][1] = v_y.Y;    ret[1][2] = v_z.Y;    ret[1][3] = 0.f;
        ret[2][0] = v_x.Z;    ret[2][1] = v_y.Z;    ret[2][2] = v_z.Z;    ret[2][3] = 0.f;
        ret[3][0] = -xDotEye; ret[3][1] = -yDotEye; ret[3][2] = -zDotEye; ret[3][3] = 1.f;

        return ret;
    }

    Matrix Matrix::CreateProjectionMatrix(float fovy, float aspect, float zNear, float zFar)
    {
        Matrix ret(4, 4);

        for (int y = 0; y < 4; ++y)
            for (int x = 0; x < 4; ++x)
                ret[y][x] = 0.f;

        const float h = 1.f / tanf(0.5f * fovy);
        const float w = h * aspect;

        ret[0][0] = w;
        ret[1][1] = h;
        ret[2][2] = zFar / (zNear - zFar);
        ret[2][3] = -1.f;
        ret[3][2] = zNear*zFar / (zNear - zFar);

        return ret;
    }

    Matrix Matrix::Transpose(const Matrix &in)
    {
        Matrix ret(in.Columns, in.Rows);

        for (int r = 0; r < in.Rows; ++r)
            for (int c = 0; c < in.Columns; ++c)
                ret[c][r] = in[r][c];

        return ret;
    }

    Matrix operator *(const Matrix &a, const Matrix &b)
    {
        if (a.Columns != b.Rows)
            throw "Invalid matrix multiplication";

        Matrix ret(a.Rows, b.Columns);

        for (int r = 0; r < a.Rows; ++r)
            for (int c = 0; c < b.Columns; ++c)
            {
                ret[r][c] = 0.f;

                for (int i = 0; i < a.Columns; ++i)
                    ret[r][c] += a.matrix[r][i] * b[i][c];
            }

        return ret;
    }

    template <typename T>
    bool operator < (const Vector3<T> &a, const Vector3<T> &b)
    {
        return a.X < b.X || a.Y < b.Y || a.Z < b.Z;
    }
}