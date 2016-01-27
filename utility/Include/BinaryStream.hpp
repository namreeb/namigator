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
    unsigned long m_position;

    public:
    BinaryStream(std::vector<char> &buffer, unsigned long bufferLength);
    BinaryStream(unsigned long bufferLength = 4096);

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

    void Write(const char *data, int length);
    void ReadBytes(void *dest, unsigned long length);
    std::string ReadCString();

    size_t Length() const;
    unsigned long GetPosition() const;

    void SetPosition(unsigned long position);
    void Slide(long offset);
};
}