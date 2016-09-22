#include "utility/Include/BinaryStream.hpp"
#include "utility/Include/Exception.hpp"

#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
#include <cstdint>

#define DEFAULT_BUFFER_LENGTH 4096

namespace utility
{
BinaryStream::BinaryStream(std::vector<char> &buffer) : m_buffer(std::move(buffer)), m_position(0L)
{
    assert(!!m_buffer.size());
}

BinaryStream::BinaryStream() : m_buffer(DEFAULT_BUFFER_LENGTH), m_position(0L) {}

void BinaryStream::Write(const char *data, size_t length)
{
    if (m_position + length > m_buffer.size())
        m_buffer.resize(2 * m_buffer.size());

    memcpy(&m_buffer[m_position], data, length);
    m_position += length;
}

std::string BinaryStream::ReadCString()
{
    std::string ret;

    for (auto c = Read<std::int8_t>(); !!c; c = Read<std::int8_t>())
        ret += c;

    return ret;
}

void BinaryStream::ReadBytes(void *dest, size_t length)
{
    memcpy(dest, &m_buffer[m_position], length);
    m_position += length;
}

size_t BinaryStream::Length() const
{
    return m_buffer.size();
}

size_t BinaryStream::GetPosition() const
{
    return m_position;
}

void BinaryStream::SetPosition(size_t position)
{
    m_position = position;
}

void BinaryStream::Slide(int offset)
{
    m_position += offset;
}

bool BinaryStream::GetChunkLocation(const std::string &chunkName, size_t &result) const
{
    return GetChunkLocation(chunkName, 0, result);
}

#define VALID_CHUNK_CHAR(x) ((x >= 'A' && x <= 'Z') || x == '2')

bool BinaryStream::GetChunkLocation(const std::string &chunkName, size_t startLoc, size_t &result) const
{
    size_t p = 0;

    // find first chunk of any type
    for (size_t i = startLoc; i < m_buffer.size(); ++i)
    {
        if ((i + 4) > m_buffer.size())
            return false;

        const char *currentChunk = &m_buffer[i];

        if (VALID_CHUNK_CHAR(currentChunk[0]) && VALID_CHUNK_CHAR(currentChunk[1]) && VALID_CHUNK_CHAR(currentChunk[2]) && currentChunk[3] == 'M')
        {
            p = i;
            break;
        }
    }

    for (auto i = p; i < m_buffer.size(); )
    {
        const char *currentChunk = &m_buffer[i];

        if (chunkName[0] == currentChunk[3] &&
            chunkName[1] == currentChunk[2] &&
            chunkName[2] == currentChunk[1] &&
            chunkName[3] == currentChunk[0])
        {
            result = i;
            return true;
        }

        unsigned int size = *reinterpret_cast<const unsigned int *>(&m_buffer[i + 4]);
        i += size + 8;
    }

    return false;
}
}