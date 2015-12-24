#include <string>
#include <vector>
#include <algorithm>

#include "BinaryStream.hpp"
#include "Misc.hpp"

namespace utility
{
    BinaryStream::BinaryStream(std::vector<char> &buffer, unsigned long bufferLength) : _buffer(std::move(buffer)), _position(0L)
    {
        if (!bufferLength)
            throw "Empty utility::BinaryStream";
    }

    BinaryStream::BinaryStream(unsigned long bufferLength) : _buffer(bufferLength), _position(0L) {}

    void utility::BinaryStream::Write(char *data, int length)
    {
        if (_position + length > _buffer.size())
            _buffer.resize((int)(_buffer.size() * 1.5));

        memcpy((char *)(&_buffer[0] + _position), data, length);
        _position += length;
    }

    std::string utility::BinaryStream::ReadCString()
    {
        std::string ret;

        for (char c = Read<char>(); c != '\0'; c = Read<char>())
            ret += c;

        return ret;
    }

    void utility::BinaryStream::ReadBytes(void *dest, unsigned long length)
    {
        std::copy(&_buffer[_position], &_buffer[_position + length], static_cast<char *>(dest));

        _position += length;
    }
};