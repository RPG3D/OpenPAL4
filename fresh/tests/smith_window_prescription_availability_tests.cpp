#include "pal4/smith_window.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowPrescriptionAvailabilityRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t list = 0x2000u;
    std::int32_t rows = 2;
    std::int32_t columns = 3;
    std::uint32_t mode = 0u;
    std::uint32_t registrations = 0u;
    std::uint32_t requirement_checks = 0u;
    std::vector<std::uint32_t> checked_ids;
    std::vector<std::array<std::uint8_t, 3>> disabled{
        {9u, 9u, 9u}, {9u, 9u, 9u}};

    static std::uint32_t Cell(std::int32_t row,
                              std::int32_t column) {
        return 0x3000u + static_cast<std::uint32_t>(row) * 0x1000u +
               static_cast<std::uint32_t>(column) * 0x100u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == object + 0x44u) return list;
        if (address == object + 0xb4u) return mode;
        if (address == Cell(0, 0) + 0x138u) return 10u;
        Check(address == Cell(1, 0) + 0x138u,
              "first-column recipe ID offset");
        return 20u;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        for (std::int32_t row = 0; row < rows; ++row) {
            for (std::int32_t column = 0; column < columns; ++column) {
                if (address == Cell(row, column) + 0x1a5u) {
                    disabled[row][column] = value;
                    return;
                }
            }
        }
        throw std::runtime_error("availability bit target");
    }
    std::int32_t RowCount(std::uint32_t target) override {
        Check(target == list, "row-count list");
        return rows;
    }
    std::int32_t ColumnCount(std::uint32_t target) override {
        Check(target == list, "column-count list");
        return columns;
    }
    std::uint32_t ItemAt(std::uint32_t target,
                          std::int32_t row,
                          std::int32_t column) override {
        Check(target == list && row >= 0 && row < rows &&
                  column >= 0 && column < 3,
              "grid lookup arguments");
        return Cell(row, column);
    }
    void RegisterSmithSystem() override { ++registrations; }
    bool CraftingRequirementsMet(std::uint32_t recipe_id) override {
        ++requirement_checks;
        checked_ids.push_back(recipe_id);
        return recipe_id == 20u;
    }
};

void CheckModeFamily() {
    for (const auto mode : {0u, 1u, 2u}) {
        Runtime runtime;
        runtime.mode = mode;
        Check(pal4::SmithWindow::RefreshPrescriptionAvailability(
                  Runtime::object, runtime) == 2 &&
                  runtime.registrations == 2u &&
                  runtime.requirement_checks == 2u &&
                  runtime.checked_ids ==
                      (std::vector<std::uint32_t>{10u, 20u}) &&
                  runtime.disabled ==
                      (std::vector<std::array<std::uint8_t, 3>>{
                          {1u, 1u, 1u}, {0u, 0u, 0u}}),
              "modes zero through two set every cell by row requirement");
    }
    for (const auto mode : {3u, 0xffffffffu}) {
        Runtime runtime;
        runtime.mode = mode;
        Check(pal4::SmithWindow::RefreshPrescriptionAvailability(
                  Runtime::object, runtime) == 2 &&
                  runtime.registrations == 0u &&
                  runtime.requirement_checks == 0u &&
                  runtime.disabled ==
                      (std::vector<std::array<std::uint8_t, 3>>{
                          {0u, 0u, 0u}, {0u, 0u, 0u}}),
              "unsigned modes above two skip Smith requirement checks");
    }
}

void CheckEmptyDimensions() {
    Runtime no_rows;
    no_rows.rows = 0;
    Check(pal4::SmithWindow::RefreshPrescriptionAvailability(
              Runtime::object, no_rows) == 0 &&
              no_rows.registrations == 0u,
          "zero rows return zero");
    Runtime negative_rows;
    negative_rows.rows = -1;
    Check(pal4::SmithWindow::RefreshPrescriptionAvailability(
              Runtime::object, negative_rows) == 0,
          "negative row count does not iterate");
    Runtime no_columns;
    no_columns.columns = 0;
    Check(pal4::SmithWindow::RefreshPrescriptionAvailability(
              Runtime::object, no_columns) == 2 &&
              no_columns.requirement_checks == 2u &&
              no_columns.disabled[0][0] == 9u,
          "zero columns still check row requirements but write no cells");
}

}  // namespace

int main() {
    try {
        CheckModeFamily();
        CheckEmptyDimensions();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
