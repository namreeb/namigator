#include "utility/Include/BinaryStream.hpp"
#include "utility/Include/Exception.hpp"

#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
#include <cstdint>

namespace utility
{
BinaryStream::BinaryStream(std::vector<char> &buffer) : m_buffer(std::move(buffer)), m_rpos(0), m_wpos(0) {}

BinaryStream::BinaryStream(size_t length) : m_buffer(length), m_rpos(0), m_wpos(0) {}

BinaryStream::BinaryStream(std::istream &stream) : m_rpos(0), m_wpos(0)
{
    stream.seekg(0, std::ios::end);
    auto const size = stream.tellg();
    stream.seekg(0, std::ios::beg);

    m_buffer.resize(size);
    stream.read(&m_buffer[0], m_buffer.size());
}

BinaryStream::BinaryStream(BinaryStream &&other) noexcept : m_buffer(std::move(other.m_buffer)), m_rpos(other.m_rpos), m_wpos(other.m_wpos)
{
    other.m_rpos = other.m_wpos = 0;
}

BinaryStream& BinaryStream::operator = (BinaryStream&& other) noexcept
{
    m_buffer = std::move(other.m_buffer);
    m_rpos = other.m_rpos;
    m_wpos = other.m_wpos;
    other.m_rpos = other.m_wpos = 0;
    return *this;
}

std::string BinaryStream::ReadCString()
{
    std::string ret;

    for (auto c = Read<std::int8_t>(); !!c; c = Read<std::int8_t>())
        ret += c;

    return ret;
}

void BinaryStream::Write(const void *data, size_t length)
{
    Write(m_wpos, data, length);
    m_wpos += length;
}

void BinaryStream::Write(size_t position, const void *data, size_t length)
{
    if (position + length > m_buffer.size())
    {
        auto const newSize = (std::max)(2 * m_buffer.size(), m_buffer.size() + length);
        m_buffer.resize(newSize);
    }

    memcpy(&m_buffer[position], data, length);
}

void BinaryStream::Append(const BinaryStream &other)
{
    if (other.m_buffer.size() > 0 && other.m_wpos > 0)
        Write(&other.m_buffer[0], other.m_wpos);
}

void BinaryStream::ReadBytes(void *dest, size_t length)
{
    if (length == 0)
        return;

    if (m_rpos + length > m_buffer.size())
        throw std::domain_error("Read past end of buffer");

    memcpy(dest, &m_buffer[m_rpos], length);
    m_rpos += length;
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

std::ostream & operator << (std::ostream &stream, const BinaryStream &data)
{
    stream.write(&data.m_buffer[0], data.m_wpos);
    return stream;
}
}