#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>

namespace utility
{
class BinaryStream
{
    private:
        std::vector<char> m_buffer;
        size_t m_position;

    public:
        BinaryStream(std::vector<char> &buffer);
        BinaryStream();

        template <typename T> T Read()
        {
            T *pos = reinterpret_cast<T *>(&m_buffer[m_position]);
            m_position += sizeof(T);

            return *pos;
        }

        template <typename T> void Write(T data)
        {
            if (m_position + sizeof(T) > m_buffer.size())
                m_buffer.resize(2 * m_buffer.size());

            T *pos = reinterpret_cast<T *>(&m_buffer[m_position]);
            *pos = data;

            m_position += sizeof(T);
        }

        void Write(const char *data, size_t length);
        void ReadBytes(void *dest, size_t length);
        std::string ReadCString();

        size_t Length() const;
        size_t GetPosition() const;

        void SetPosition(size_t position);
        void Slide(int offset);

        bool GetChunkLocation(const std::string &chunkName, size_t &result) const;
        bool GetChunkLocation(const std::string &chunkName, size_t startLoc, size_t &result) const;
};
}