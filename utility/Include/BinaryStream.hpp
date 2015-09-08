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
            std::vector<char> _buffer;
            unsigned long _position;

        public:
            BinaryStream(std::vector<char> &buffer, unsigned long bufferLength);
            BinaryStream(unsigned long bufferLength = 4096);
            ~BinaryStream();

            template <typename T> T Read()
            {
                char *pos = (char *)&_buffer[0] + _position;
                T ret = *(T *)pos;
                _position += sizeof(T);

                return ret;
            }

            void Write(char *data, int length);

            template <typename T> void Write(T data)
            {
                if (_position + sizeof(T) > _buffer.size())
                    _buffer.resize((int)(_buffer.size() * 1.5));

                T *pos = (T *)((char *)&_buffer[0] + _position)
                *pos = data;

                _position += sizeof(T);
            }

            void ReadBytes(void *dest, unsigned long length);
            std::string ReadCString();

            template <typename T>
            T *AllocateAndReadStruct()
            {
                T *ret(new T);

                memcpy(ret, (char *)&_buffer[0] + _position, sizeof(T));
                _position += sizeof(T);

                return ret;
            }

            inline unsigned long Length() { return _buffer.size(); }
            inline void SetPosition(unsigned long position) { _position = position; }
            inline unsigned long GetPosition() { return _position; }
            inline void Slide(long offset) { _position += offset; }
    };
};