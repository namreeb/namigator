#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <type_traits>
#include <ostream>
#include <fstream>

namespace utility
{
class BinaryStream
{
    private:
        static constexpr size_t DEFAULT_BUFFER_LENGTH = 4096;

        friend std::ostream & operator << (std::ostream &, const BinaryStream &);

        std::vector<char> m_buffer;
        size_t m_rpos, m_wpos;

    public:
        BinaryStream(std::vector<char> &buffer);
        BinaryStream(size_t length = DEFAULT_BUFFER_LENGTH);
        BinaryStream(std::istream &stream);
        BinaryStream(BinaryStream &&other) noexcept;

        BinaryStream& operator = (BinaryStream&& other) noexcept;

        template <typename T> T Read()
        {
            static_assert(std::is_pod<T>::value, "T must be POD");
            static_assert(sizeof(T) <= 4, "Stack return read is meant only for small values");

            T ret;
            Read(ret);
            return ret;
        }

        template <typename T> void Read(T &out)
        {
            static_assert(std::is_pod<T>::value, "T must be POD");

            ReadBytes(&out, sizeof(T));
        }

        template <typename T> void Write(const T& data)
        {
            static_assert(std::is_pod<T>::value, "T must be POD");

            Write(&data, sizeof(T));
        }

        template <typename T> void Write(size_t position, T data)
        {
            Write(position, &data, sizeof(T));
        }

        void Write(const void *data, size_t length);
        void Write(size_t position, const void *data, size_t length);
        void ReadBytes(void *dest, size_t length);
        std::string ReadCString();

        void Append(const BinaryStream &other);

        size_t rpos() const { return m_rpos; }
        void rpos(size_t pos) { m_rpos = pos; }

        size_t wpos() const { return m_wpos; }
        void wpos(size_t pos) { m_wpos = pos; }

        bool GetChunkLocation(const std::string &chunkName, size_t &result) const;
        bool GetChunkLocation(const std::string &chunkName, size_t startLoc, size_t &result) const;
};

template <typename T>
BinaryStream & operator << (BinaryStream &stream, T data)
{
    static_assert(std::is_pod<T>::value, "T must be POD");

    stream.Write(data);
    return stream;
}

template <typename T>
BinaryStream & operator >> (BinaryStream &stream,  T &data)
{
    static_assert(std::is_pod<T>::value, "T must be POD");

    stream.Read(data);
    return stream;
}

std::ostream & operator << (std::ostream &stream, const BinaryStream &data);
}