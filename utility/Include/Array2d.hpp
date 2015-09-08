#pragma once

// XXX - rework this to use Matrix type

namespace utility
{
    template <typename T>
    class Array2d
    {
        public:
            T *Data;
            const int Width;
            const int Rows;

            Array2d(int rows, int columns) : Width(columns), Rows(rows), Data(new T[rows*columns]) {}

            ~Array2d()
            {
                delete[] Data;
            }

            inline void Set(int row, int column, T value)
            {
                *(Data + Width * row + column) = value;
            }

            inline T Get(int row, int column) const
            {
                return *(Data + Width * row + column);
            }
    };
}