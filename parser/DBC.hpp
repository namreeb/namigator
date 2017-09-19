#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace parser
{
class DBC
{
    private:
        static constexpr std::uint32_t Magic = 'CBDW';

        std::vector<std::uint32_t> m_data;
        std::vector<std::uint8_t> m_string;

        size_t m_recordCount;
        size_t m_fieldCount;

    public:
        DBC(const std::string &filename);

        std::uint32_t GetField(int row, int column) const;
        std::string GetStringField(int row, int column) const;

        size_t RecordCount() const { return m_recordCount; }
};
}
