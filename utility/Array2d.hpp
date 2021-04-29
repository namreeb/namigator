#pragma once

#include <cassert>
#include <vector>

namespace utility
{
template <typename T>
class Array2d
{
private:
    std::vector<T> Data;

public:
    const int Width;
    const int Rows;

    Array2d(int rows, int columns)
        : Width(columns), Rows(rows), Data(rows * columns)
    {
    }

    void Set(int row, int column, T value)
    {
        Data[Width * row + column] = value;
    }

    T Get(int row, int column) const
    {
        assert(row < Rows && column < Width);

        return Data[Width * row + column];
    }
};
} // namespace utility