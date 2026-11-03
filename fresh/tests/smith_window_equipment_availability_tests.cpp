#include "pal4/smith_window.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowEquipmentAvailabilityRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t list = 0x2000u;
    std::uint32_t begin = 0x9000u;
    std::uint32_t end = 0x9008u;
    std::uint32_t mode = 1u;
    std::int32_t rows = 2;
    std::int32_t columns = 2;
    std::uint32_t registrations = 0u;
    std::uint32_t hover_calls = 0u;
    std::vector<std::pair<std::uint32_t, bool>> visibility;
    std::vector<std::array<std::uint8_t, 2>> disabled{
        {9u, 9u}, {9u, 9u}};

    static std::uint32_t Cell(std::int32_t row,
                              std::int32_t column) {
        return 0x3000u + static_cast<std::uint32_t>(row) * 0x1000u +
               static_cast<std::uint32_t>(column) * 0x100u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == object + 0xccu) return begin;
        if (address == object + 0xd0u) return end;
        if (address == object + 0x58u) return 0x5000u;
        if (address == object + 0x5cu) return 0x6000u;
        if (address == object + 0xb4u) return mode;
        if (address == object + 0x40u) return list;
        if (address == Cell(0, 0) + 0x138u) return 10u;
        Check(address == Cell(1, 0) + 0x138u,
              "equipment ID offset");
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
        throw std::runtime_error("equipment availability bit target");
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        Check(window == 0x5000u || window == 0x6000u,
              "page button window");
        visibility.emplace_back(window, visible);
    }
    std::int32_t RowCount(std::uint32_t target) override {
        Check(target == list, "equipment row-count list");
        return rows;
    }
    std::int32_t ColumnCount(std::uint32_t target) override {
        Check(target == list, "equipment column-count list");
        return columns;
    }
    std::int32_t PrescriptionHoverItemId(std::uint32_t owner) override {
        Check(owner == object, "prescription hover owner");
        ++hover_calls;
        return 77;
    }
    std::uint32_t ItemAt(std::uint32_t target,
                          std::int32_t row,
                          std::int32_t column) override {
        Check(target == list && row >= 0 && row < rows &&
                  column >= 0 && column < 2,
              "equipment grid lookup");
        return Cell(row, column);
    }
    void RegisterSmithSystem() override { ++registrations; }
    std::int32_t EquipmentProperty(std::uint32_t id) override {
        Check(id == 10u || id == 20u, "equipment property ID");
        return id == 10u ? 0 : 5;
    }
    bool EquipmentPropertySufficient(
        std::int32_t prescription_id,
        std::uint32_t equipment_id) override {
        Check(prescription_id == 77 &&
                  (equipment_id == 10u || equipment_id == 20u),
              "property sufficiency IDs");
        return equipment_id == 20u;
    }
};

void CheckVisibilityAndModes() {
    Runtime mode_zero;
    mode_zero.mode = 0u;
    pal4::SmithWindow::RefreshEquipmentAvailability(
        Runtime::object, mode_zero);
    Check(mode_zero.visibility == (std::vector<std::pair<std::uint32_t, bool>>{
              {0x5000u, true}, {0x6000u, true}}) &&
              mode_zero.hover_calls == 0u &&
              mode_zero.disabled[0][0] == 9u,
          "mode zero updates only page-button visibility");

    for (const auto mode : {1u, 2u}) {
        Runtime runtime;
        runtime.mode = mode;
        pal4::SmithWindow::RefreshEquipmentAvailability(
            Runtime::object, runtime);
        Check(runtime.visibility ==
                  (std::vector<std::pair<std::uint32_t, bool>>{
                      {0x5000u, true}, {0x6000u, true}}) &&
                  runtime.hover_calls == 1u &&
                  runtime.registrations == 2u &&
                  runtime.disabled ==
                      (std::vector<std::array<std::uint8_t, 2>>{
                          {1u, 1u}, {0u, 0u}}),
              "modes one and two disable unavailable equipment rows");
    }
    Runtime other;
    other.mode = 3u;
    pal4::SmithWindow::RefreshEquipmentAvailability(
        Runtime::object, other);
    Check(other.registrations == 0u && other.hover_calls == 1u &&
              other.disabled ==
                  (std::vector<std::array<std::uint8_t, 2>>{
                      {0u, 0u}, {0u, 0u}}),
          "other nonzero modes mark every equipment cell available");
}

void CheckArrayAndEmptyList() {
    Runtime absent;
    absent.begin = 0u;
    absent.mode = 0u;
    pal4::SmithWindow::RefreshEquipmentAvailability(
        Runtime::object, absent);
    Check(absent.visibility == (std::vector<std::pair<std::uint32_t, bool>>{
              {0x5000u, false}, {0x6000u, false}}),
          "null category array hides both page buttons");
    Runtime single;
    single.end = single.begin + 4u;
    single.mode = 0u;
    pal4::SmithWindow::RefreshEquipmentAvailability(
        Runtime::object, single);
    Check(!single.visibility[0].second && !single.visibility[1].second,
          "single category hides page buttons");
    Runtime empty;
    empty.rows = 0;
    pal4::SmithWindow::RefreshEquipmentAvailability(
        Runtime::object, empty);
    Check(empty.hover_calls == 1u && empty.registrations == 0u &&
              empty.disabled[0][0] == 9u,
          "empty equipment list still reads prescription hover");
}

}  // namespace

int main() {
    try {
        CheckVisibilityAndModes();
        CheckArrayAndEmptyList();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
