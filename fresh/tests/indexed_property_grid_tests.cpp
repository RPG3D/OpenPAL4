#include "pal4/indexed_property_grid.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void TestConstructAndResize() {
    pal4::IndexedPropertyGrid grid(0xa5u);
    Check(grid.InitialMarker() == 0xa5u && grid.RowCount() == 0 &&
              grid.ColumnCount() == 0 && grid.Rows().empty(),
          "constructor preserves unknown source byte and starts empty");
    grid.Resize(3, 4);
    Check(grid.RowCount() == 3 && grid.ColumnCount() == 4 &&
              grid.Rows().size() == 3u, "dimension fields and row count");
    for (const auto& row : grid.Rows()) {
        Check(row.marker == 3u && row.values == std::vector<std::int32_t>(4, 0),
              "row marker and zero column template");
    }
    static_cast<void>(grid.Set(1, 2, 99));
    grid.Resize(2, 1);
    Check(grid.RowCount() == 2 && grid.ColumnCount() == 1 &&
              grid.Get(1, 0) == 0 && grid.Rows()[0].marker == 2u,
          "resize discards old rows and creates new zero values");
    grid.Resize(2, 0);
    Check(grid.Rows().size() == 2u && grid.Rows()[0].values.empty(),
          "zero columns still constructs requested rows");
    grid.Resize(-3, 2);
    Check(grid.RowCount() == -3 && grid.ColumnCount() == 2 &&
              grid.Rows().empty(), "negative row count skips insertion");
    try {
        grid.Resize(1, -1);
        throw std::runtime_error("expected invalid column count");
    } catch (const std::length_error&) {
    }
}

void TestIndexedAccess() {
    pal4::IndexedPropertyGrid grid(0);
    grid.Resize(2, 3);
    const auto row_address = grid.Set(1, 2, 1234);
    Check(row_address == reinterpret_cast<std::intptr_t>(
              grid.Rows()[1].values.data()),
          "valid set returns row's internal value address");
    Check(grid.Get(1, 2) == 1234 && grid.Get(0, 0) == 0,
          "indexed values remain independent");
    for (const auto [row, column] :
         std::array<std::array<std::int32_t, 2>, 5>{{
             {2, 0}, {0, 3}, {-1, 0}, {0, -1}, {-1, -1}}}) {
        Check(grid.Get(row, column) == -2, "invalid read returns -2");
        Check(grid.Set(row, column, 77) == row,
              "invalid write returns original row argument");
    }
    Check(grid.Get(1, 2) == 1234, "invalid writes leave contents intact");
}

void TestCoordinates() {
    const auto value = pal4::IndexedPropertyGrid::Coordinates(
        {10.0f, -8.0f}, {2.0f, 4.0f}, 15.9f, 3.0f);
    Check(value[0] == 2 && value[1] == 2,
          "x and y use separate origins and scales with truncation");
    const auto negative = pal4::IndexedPropertyGrid::Coordinates(
        {10.0f, 20.0f}, {2.0f, 3.0f}, 7.0f, 16.0f);
    Check(negative[0] == -1 && negative[1] == -1,
          "negative quotients truncate toward zero");
    const auto invalid = pal4::IndexedPropertyGrid::Coordinates(
        {0.0f, 0.0f}, {0.0f, 1.0f}, 1.0f,
        std::numeric_limits<float>::infinity());
    Check(invalid[0] == 0 && invalid[1] == 0,
          "invalid x87 64-bit conversions leave zero low word");
    const auto wrapped = pal4::IndexedPropertyGrid::Coordinates(
        {0.0f, 0.0f}, {1.0f, 1.0f}, 2147483648.0f, 4294967296.0f);
    Check(wrapped[0] == std::numeric_limits<std::int32_t>::min() &&
              wrapped[1] == 0,
          "CRT_Ftol return uses low 32 bits of truncated 64-bit integer");
}

void TestCleanupAndInsertion() {
    pal4::IndexedPropertyGrid grid(0);
    grid.Resize(3, 2);
    static_cast<void>(grid.Set(0, 0, 11));
    static_cast<void>(grid.Set(1, 0, 22));
    static_cast<void>(grid.Set(2, 0, 33));
    grid.CleanupRange(1, 2);
    Check(grid.Rows().size() == 3u &&
              grid.Rows()[0].values[0] == 11 &&
              grid.Rows()[1].values.empty() &&
              grid.Rows()[2].values[0] == 33,
          "range cleanup frees only selected row payloads");
    try {
        grid.CleanupRange(3, 4);
        throw std::runtime_error("expected invalid cleanup range");
    } catch (const std::out_of_range&) {
    }

    const pal4::IndexedPropertyRow source{0x7au, {5, 6}};
    grid.InsertRepeated(1, 2, source);
    Check(grid.Rows().size() == 5u &&
              grid.Rows()[1].marker == 0x7au &&
              grid.Rows()[2].values == source.values &&
              grid.Rows()[3].values.empty(),
          "middle insertion shifts old rows and deep copies template");
    static_cast<void>(grid.Rows());
    grid.InsertRepeated(grid.Rows().size(), 1, grid.Rows()[0]);
    Check(grid.Rows().size() == 6u && grid.Rows().back().values[0] == 11,
          "append supports a source inside the same collection");
    grid.InsertRepeated(0, 0, source);
    Check(grid.Rows().size() == 6u, "zero insertion is inert");
    try {
        grid.InsertRepeated(7, 1, source);
        throw std::runtime_error("expected invalid insert position");
    } catch (const std::out_of_range&) {
    }

    grid.Cleanup();
    Check(grid.Rows().empty() && grid.RowCount() == 3 &&
              grid.ColumnCount() == 2,
          "full cleanup resets storage but preserves dimension metadata");
    grid.Cleanup();
    Check(grid.Rows().empty(), "repeat cleanup is inert");
}

void TestRowCopy() {
    const pal4::IndexedPropertyRow source{0x72u, {11, 22, 33}};
    pal4::IndexedPropertyRow destination{0x19u, {1}};
    pal4::IndexedPropertyGrid::CopyRow(nullptr, source);
    pal4::IndexedPropertyGrid::CopyRow(&destination, source);
    Check(destination.marker == source.marker &&
              destination.values == source.values &&
              destination.values.data() != source.values.data(),
          "row copy owns independent DWORD storage");
    destination.values[0] = 90;
    Check(source.values[0] == 11, "row copy does not alias source");
    pal4::IndexedPropertyGrid::CopyRow(&destination, destination);
    Check(destination.values[0] == 90, "self copy preserves row");
    const pal4::IndexedPropertyRow empty{0x33u, {}};
    pal4::IndexedPropertyGrid::CopyRow(&destination, empty);
    Check(destination.marker == 0x33u && destination.values.empty(),
          "empty source clears prior owned storage");
}

}  // namespace

int main() {
    try {
        TestConstructAndResize();
        TestIndexedAccess();
        TestCoordinates();
        TestCleanupAndInsertion();
        TestRowCopy();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
