#include "utility/Include/BinaryStream.hpp"
#include "utility/Include/Misc.hpp"

#include <string>
#include <vector>
#include <algorithm>

namespace utility
{
BinaryStream::BinaryStream(std::vector<char> &buffer, unsigned long bufferLength) : m_buffer(std::move(buffer)), m_position(0L)
{
    if (!bufferLength)
        THROW("Empty BinaryStream");
}

BinaryStream::BinaryStream(unsigned long bufferLength) : m_buffer(bufferLength), m_position(0L)
{
    if (!bufferLength)
        THROW("Empty BinaryStream");
}

void BinaryStream::Write(const char *data, int length)
{
    if (m_position + length > m_buffer.size())
        m_buffer.resize(2 * m_buffer.size());

    memcpy(&m_buffer[m_position], data, length);
    m_position += length;
}

std::string BinaryStream::ReadCString()
{
    std::string ret;

    for (char c = Read<char>(); c != '\0'; c = Read<char>())
        ret += c;

    return ret;
}

void BinaryStream::ReadBytes(void *dest, unsigned long length)
{
    std::copy(&m_buffer[m_position], &m_buffer[m_position + length], static_cast<char *>(dest));

    m_position += length;
}

size_t BinaryStream::Length() const
{
    return m_buffer.size();
}

unsigned long BinaryStream::GetPosition() const
{
    return m_position;
}

void BinaryStream::SetPosition(unsigned long position)
{
    m_position = position;
}

void BinaryStream::Slide(long offset)
{
    m_position += offset;
}
}