#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace pal4 {

struct IndexedPropertyRow {
    std::uint8_t marker = 0;
    std::vector<std::int32_t> values;
};

class IndexedPropertyGrid final {
public:
    // IDA 0x5aa7a0: the source byte is read from an unwritten stack slot.
    explicit IndexedPropertyGrid(std::uint8_t initial_marker);

    // IDA 0x5aa7f0/0x5aa950/0x5aa980.
    void Resize(std::int32_t rows, std::int32_t columns);
    [[nodiscard]] std::intptr_t Set(std::int32_t row,
                                    std::int32_t column,
                                    std::int32_t value);
    [[nodiscard]] std::int32_t Get(std::int32_t row,
                                   std::int32_t column) const;

    // IDA 0x5aa9c0: x and y use their respective origin and scale.
    [[nodiscard]] static std::array<std::int32_t, 2> Coordinates(
        const std::array<float, 2>& origin,
        const std::array<float, 2>& scale, float x, float y);

    // IDA 0x5aaa00/0x5aaa50/0x5aaa90.
    void Cleanup();
    void CleanupRange(std::size_t first, std::size_t last);
    void InsertRepeated(std::size_t index, std::size_t count,
                        const IndexedPropertyRow& source);
    // IDA 0x5aad10: the destination owns an independent value array.
    static void CopyRow(IndexedPropertyRow* destination,
                        const IndexedPropertyRow& source);

    [[nodiscard]] std::uint8_t InitialMarker() const { return marker_; }
    [[nodiscard]] std::int32_t RowCount() const { return row_count_; }
    [[nodiscard]] std::int32_t ColumnCount() const { return column_count_; }
    [[nodiscard]] const std::vector<IndexedPropertyRow>& Rows() const {
        return rows_;
    }

private:
    std::uint8_t marker_ = 0;
    std::vector<IndexedPropertyRow> rows_;
    std::int32_t row_count_ = 0;
    std::int32_t column_count_ = 0;
};

}  // namespace pal4
