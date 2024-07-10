#include "utility/BinaryStream.hpp"

#include "utility/Exception.hpp"
#include "utility/miniz.c"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace utility
{
BinaryStream::BinaryStream(
    std::shared_ptr<std::vector<std::uint8_t>> shared_buffer)
    : m_sharedBuffer(shared_buffer), m_rpos(0), m_wpos(m_sharedBuffer->size())
{
}

BinaryStream::BinaryStream(std::vector<std::uint8_t>& buffer)
    : m_buffer(std::move(buffer)), m_rpos(0), m_wpos(m_buffer.size())
{
}

BinaryStream::BinaryStream(size_t length)
    : m_buffer(length), m_rpos(0), m_wpos(0)
{
}

BinaryStream::BinaryStream(const std::filesystem::path& path)
    : m_rpos(0), m_wpos(0)
{
    std::ifstream stream(path, std::ifstream::binary);

    if (stream.fail())
        THROW(Result::FAILED_TO_OPEN_FILE_FOR_BINARY_STREAM);

    stream.seekg(0, std::ios::end);
    m_wpos = static_cast<size_t>(stream.tellg());
    stream.seekg(0, std::ios::beg);

    m_buffer.resize(m_wpos);

    if (m_wpos > 0)
        stream.read(reinterpret_cast<char*>(&m_buffer[0]), m_buffer.size());
}

BinaryStream::BinaryStream(BinaryStream&& other) noexcept
    : m_buffer(std::move(other.m_buffer)),
      m_sharedBuffer(std::move(other.m_sharedBuffer)), m_rpos(other.m_rpos),
      m_wpos(other.m_wpos)
{
    other.m_rpos = other.m_wpos = 0;
}

BinaryStream& BinaryStream::operator=(BinaryStream&& other) noexcept
{
    if (other.m_sharedBuffer)
        m_sharedBuffer = std::move(other.m_sharedBuffer);
    else
        m_buffer = std::move(other.m_buffer);
    m_rpos = other.m_rpos;
    m_wpos = other.m_wpos;
    other.m_rpos = other.m_wpos = 0;
    return *this;
}

std::string BinaryStream::ReadString()
{
    std::string ret;

    for (auto c = Read<std::int8_t>(); !!c; c = Read<std::int8_t>())
        ret += c;

    return ret;
}

std::string BinaryStream::ReadString(size_t length)
{
    std::string ret;

    for (auto i = 0u; i < length; ++i)
        ret += Read<std::int8_t>();

    return ret;
}

void BinaryStream::Write(const void* data, size_t length)
{
    Write(m_wpos, data, length);
    m_wpos += length;
}

void BinaryStream::Write(size_t position, const void* data, size_t length)
{
    // without this, if the buffer is full, the memcpy() call below will read
    // past the end of m_buffer
    if (!length)
        return;

    auto const targetBuffer = buffer();

    if (position + length > targetBuffer->size())
    {
        auto const newSize =
            (std::max)(2 * targetBuffer->size(), targetBuffer->size() + length);
        targetBuffer->resize(newSize);
    }

    memcpy(&targetBuffer->at(position), data, length);
}

void BinaryStream::WriteString(const std::string& str, size_t length)
{
    std::vector<std::uint8_t> buffer(length);
    memcpy(&buffer[0], str.c_str(), (std::min)(length - 1, str.length()));
    Write(&buffer[0], buffer.size());
}

void BinaryStream::Append(const BinaryStream& other)
{
    if (other.m_wpos > 0)
    {
        auto const sourceBuffer = other.buffer();
        Write(&sourceBuffer->at(0), other.m_wpos);
    }
}

void BinaryStream::ReadBytes(void* dest, size_t length)
{
    if (length == 0)
        return;

    auto const buff = buffer();
    if (m_rpos + length > buff->size())
        throw std::domain_error("Read past end of buffer");

    memcpy(dest, &buff->at(m_rpos), length);
    m_rpos += length;
}

bool BinaryStream::GetChunkLocation(const std::string& chunkName,
                                    size_t& result) const
{
    return GetChunkLocation(chunkName, 0, result);
}

#define VALID_CHUNK_CHAR(x) ((x >= 'A' && x <= 'Z') || x == '2')

bool BinaryStream::GetChunkLocation(const std::string& chunkName,
                                    size_t startLoc, size_t& result) const
{
    size_t p = 0;

    auto const buff = buffer();

    // find first chunk of any type
    for (size_t i = startLoc; i < buff->size(); ++i)
    {
        if ((i + 4) > buff->size())
            return false;

        auto const currentChunk = &buff->at(i);

        if (VALID_CHUNK_CHAR(currentChunk[0]) &&
            VALID_CHUNK_CHAR(currentChunk[1]) &&
            VALID_CHUNK_CHAR(currentChunk[2]) &&
            VALID_CHUNK_CHAR(currentChunk[3]))
        {
            p = i;
            break;
        }
    }

    for (auto i = p; i < buff->size();)
    {
        auto const currentChunk = &buff->at(i);

        if (chunkName[0] == currentChunk[3] &&
            chunkName[1] == currentChunk[2] &&
            chunkName[2] == currentChunk[1] && chunkName[3] == currentChunk[0])
        {
            result = i;
            return true;
        }

        auto const size =
            *reinterpret_cast<const std::uint32_t*>(&buff->at(i + 4));
        i += size + 8;
    }

    return false;
}

bool BinaryStream::IsEOF()
{
    auto const buff = buffer();
    return m_rpos == buff->size();
}

void BinaryStream::Compress()
{
    std::vector<std::uint8_t> buff(
        compressBound(static_cast<mz_ulong>(m_wpos)));
    auto newSize = static_cast<mz_ulong>(buff.size());
    auto const result =
        compress(&buff[0], &newSize,
                 reinterpret_cast<const unsigned char*>(&m_buffer[0]),
                 static_cast<mz_ulong>(m_wpos));

    if (result != MZ_OK)
        THROW(Result::BINARYSTREAM_COMPRESS_FAILED);

    m_wpos = static_cast<size_t>(newSize);
    buff.resize(m_wpos);

    m_buffer = std::move(buff);
    m_rpos = 0;
}

void BinaryStream::Decompress()
{
    std::vector<std::uint8_t> buffer(m_wpos);
    mz_stream stream;
    memset(&stream, 0, sizeof(stream));

    stream.next_in = &m_buffer[0];
    stream.avail_in = static_cast<unsigned int>(m_wpos);
    stream.next_out = &buffer[0];
    stream.avail_out = static_cast<unsigned int>(buffer.size());

    auto status = mz_inflateInit(&stream);

    if (status != MZ_OK)
        THROW(Result::MZ_INFLATEINIT_FAILED);

    do
    {
        status = mz_inflate(&stream, MZ_NO_FLUSH);

        // done decompressing? end
        if (status == MZ_STREAM_END)
            break;

        // more to decompress
        if (status == MZ_OK)
        {
            buffer.resize(2 * (std::max)(buffer.size(), static_cast<size_t>(
                                                            stream.total_out)));
            stream.next_out = &buffer[stream.total_out];
            stream.avail_out =
                static_cast<unsigned int>(buffer.size() - stream.total_out);
        }
        // unknown failure
        else
            THROW(Result::MZ_INFLATE_FAILED);
    } while (true);

    m_rpos = 0;
    m_wpos = stream.total_out;
    mz_inflateEnd(&stream);

    m_buffer = std::move(buffer);
    m_buffer.resize(m_wpos);
}

BinaryStream& operator<<(BinaryStream& stream, const std::string& str)
{
    stream.Write(str.c_str(), str.length());
    return stream;
}

BinaryStream& operator<<(BinaryStream& stream, const BinaryStream& other)
{
    stream.Write(&other.m_buffer[0], other.wpos());
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const BinaryStream& data)
{
    stream.write(reinterpret_cast<const char*>(&data.m_buffer[0]), data.m_wpos);
    return stream;
}
} // namespace utility