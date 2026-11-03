#include "pal4/indexed_property_grid.h"

#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace pal4 {
namespace {

std::int32_t Ftol(const double value) {
    if (!std::isfinite(value) ||
        value < -9223372036854775808.0 ||
        value >= 9223372036854775808.0) {
        return 0;
    }
    const auto wide = static_cast<std::int64_t>(std::trunc(value));
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(wide));
}

}  // namespace

IndexedPropertyGrid::IndexedPropertyGrid(const std::uint8_t initial_marker)
    : marker_(initial_marker) {
    Resize(0, 0);
}

void IndexedPropertyGrid::Resize(const std::int32_t rows,
                                 const std::int32_t columns) {
    if (columns < 0) throw std::length_error("negative column count");
    CleanupRange(0, rows_.size());
    rows_.clear();
    row_count_ = rows;
    column_count_ = columns;
    if (rows <= 0) return;

    IndexedPropertyRow prototype;
    prototype.marker = static_cast<std::uint8_t>(rows);
    prototype.values.resize(static_cast<std::size_t>(columns), 0);
    InsertRepeated(0, static_cast<std::size_t>(rows), prototype);
}

std::intptr_t IndexedPropertyGrid::Set(
    const std::int32_t row, const std::int32_t column,
    const std::int32_t value) {
    if (row < 0 || column < 0 || row >= row_count_ ||
        column >= column_count_) {
        return row;
    }
    auto& values = rows_[static_cast<std::size_t>(row)].values;
    values[static_cast<std::size_t>(column)] = value;
    return reinterpret_cast<std::intptr_t>(values.data());
}

std::int32_t IndexedPropertyGrid::Get(const std::int32_t row,
                                      const std::int32_t column) const {
    if (row < 0 || column < 0 || row >= row_count_ ||
        column >= column_count_) {
        return -2;
    }
    return rows_[static_cast<std::size_t>(row)]
        .values[static_cast<std::size_t>(column)];
}

std::array<std::int32_t, 2> IndexedPropertyGrid::Coordinates(
    const std::array<float, 2>& origin,
    const std::array<float, 2>& scale, const float x, const float y) {
    return {Ftol((static_cast<double>(x) - origin[0]) / scale[0]),
            Ftol((static_cast<double>(y) - origin[1]) / scale[1])};
}

void IndexedPropertyGrid::Cleanup() {
    CleanupRange(0, rows_.size());
    std::vector<IndexedPropertyRow>().swap(rows_);
}

void IndexedPropertyGrid::CleanupRange(const std::size_t first,
                                        const std::size_t last) {
    if (first > last || last > rows_.size()) {
        throw std::out_of_range("row cleanup range");
    }
    for (std::size_t index = first; index != last; ++index) {
        std::vector<std::int32_t>().swap(rows_[index].values);
    }
}

void IndexedPropertyGrid::InsertRepeated(
    const std::size_t index, const std::size_t count,
    const IndexedPropertyRow& source) {
    if (index > rows_.size()) throw std::out_of_range("row insertion index");
    IndexedPropertyRow copy;
    CopyRow(&copy, source);
    rows_.insert(rows_.begin() + static_cast<std::ptrdiff_t>(index),
                 count, copy);
}

void IndexedPropertyGrid::CopyRow(IndexedPropertyRow* destination,
                                   const IndexedPropertyRow& source) {
    if (destination == nullptr || destination == &source) return;
    destination->marker = source.marker;
    destination->values = source.values;
}

}  // namespace pal4
