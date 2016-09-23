#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <type_traits>
#include <ostream>

namespace utility
{
class BinaryStream
{
    private:
        static constexpr size_t DEFAULT_BUFFER_LENGTH = 4096;

        friend std::ostream & operator << (std::ostream &, const BinaryStream &);

        std::vector<char> m_buffer;
        size_t m_position;

    public:
        BinaryStream(std::vector<char> &buffer);
        BinaryStream(size_t length = DEFAULT_BUFFER_LENGTH);
        BinaryStream(BinaryStream &&other);

        BinaryStream& operator = (BinaryStream&& other);

        template <typename T> T Read()
        {
            T ret;
            ReadBytes(&ret, sizeof(T));
            return ret;
        }

        template <typename T> void Write(T data)
        {
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

        size_t GetPosition() const;

        void SetPosition(size_t position);
        void Slide(int offset);

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

std::ostream & operator << (std::ostream &stream, const BinaryStream &data);
}