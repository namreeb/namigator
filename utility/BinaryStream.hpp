#pragma once

#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <type_traits>
#include <vector>

namespace utility
{
class BinaryStream
{
private:
    static constexpr size_t DEFAULT_BUFFER_LENGTH = 4096;

    friend std::ostream& operator<<(std::ostream&, const BinaryStream&);
    friend BinaryStream& operator<<(BinaryStream&, const BinaryStream&);
    friend BinaryStream& operator<<(BinaryStream&, const std::string&);

    std::shared_ptr<std::vector<std::uint8_t>> m_sharedBuffer;
    std::vector<std::uint8_t> m_buffer;
    size_t m_rpos, m_wpos;

    inline std::vector<std::uint8_t>* buffer()
    {
        return m_sharedBuffer ? m_sharedBuffer.get() : &m_buffer;
    }

    inline const std::vector<std::uint8_t>* buffer() const
    {
        return m_sharedBuffer ? m_sharedBuffer.get() : &m_buffer;
    }

public:
    BinaryStream(std::shared_ptr<std::vector<std::uint8_t>> sharedBuffer);
    BinaryStream(std::vector<std::uint8_t>& buffer);
    BinaryStream(size_t length = DEFAULT_BUFFER_LENGTH);
    BinaryStream(const std::filesystem::path& path);
    BinaryStream(BinaryStream&& other) noexcept;

    BinaryStream& operator=(BinaryStream&& other) noexcept;

    template <typename T>
    T Read()
    {
        static_assert(std::is_trivially_copyable<T>::value,
                      "T must be trivially copyable");
        static_assert(sizeof(T) <= 4,
                      "Stack return read is meant only for small values");

        T ret;
        Read(ret);
        return ret;
    }

    // TODO: enable_if<is_assignable && is_trivially_copyable> for assignment
    // instead of memcpy
    template <typename T>
    void Read(T& out)
    {
        static_assert(std::is_trivially_copyable<T>::value,
                      "T must be trivially copyable");

        ReadBytes(&out, sizeof(T));
    }

    template <typename T>
    void Write(const T& data)
    {
        static_assert(std::is_trivially_copyable<T>::value,
                      "T must be trivially copyable");

        Write(&data, sizeof(T));
    }

    template <typename T>
    void Write(size_t position, T data)
    {
        Write(position, &data, sizeof(T));
    }

    void Write(const void* data, size_t length);
    void Write(size_t position, const void* data, size_t length);
    void ReadBytes(void* dest, size_t length);

    std::string ReadString();
    std::string ReadString(size_t length);

    void WriteString(const std::string& str, size_t length);

    void Append(const BinaryStream& other);

    size_t rpos() const { return m_rpos; }
    void rpos(size_t pos) { m_rpos = pos; }

    size_t wpos() const { return m_wpos; }
    void wpos(size_t pos) { m_wpos = pos; }

    bool GetChunkLocation(const std::string& chunkName, size_t& result) const;
    bool GetChunkLocation(const std::string& chunkName, size_t startLoc,
                          size_t& result) const;
    bool IsEOF();

    void Compress();
    void Decompress();
};

template <typename T>
BinaryStream& operator<<(BinaryStream& stream, T data)
{
    static_assert(std::is_trivially_copyable<T>::value,
                  "T must be trivially copyable");

    stream.Write(data);
    return stream;
}

template <typename T>
BinaryStream& operator>>(BinaryStream& stream, T& data)
{
    static_assert(std::is_trivially_copyable<T>::value,
                  "T must be trivially copyable");

    stream.Read(data);
    return stream;
}

std::ostream& operator<<(std::ostream&, const BinaryStream&);
BinaryStream& operator<<(BinaryStream&, const BinaryStream&);
BinaryStream& operator<<(BinaryStream&, const std::string&);
std::ostream& operator<<(std::ostream& stream, const BinaryStream& data);
} // namespace utility