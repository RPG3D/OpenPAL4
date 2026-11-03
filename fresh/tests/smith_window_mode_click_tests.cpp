#include "pal4/smith_window.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowModeClickRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::uint32_t mode = 0;
    std::uint32_t rows = 1;
    std::uint32_t requested = 0xffffffffu;
    std::uint32_t hovered_row = 0xffffffffu;
    std::string image;
    std::string save_key;
    std::array<bool, 3> selected{false, false, false};
    std::vector<std::string> calls;

    void ClearEquipmentSelection(std::uint32_t owner) override {
        Check(owner == object, "clear selection owner");
        calls.emplace_back("clear");
    }
    void LoadData(std::uint32_t owner,
                  std::uint32_t requested_mode) override {
        Check(owner == object, "load data owner");
        requested = requested_mode;
        calls.emplace_back("load");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == object + 0x44u) return 0x2000u;
        if (address == object + 0xb4u) return mode;
        if (address == object + 0x70u) return 0x3000u;
        if (address >= object + 0x60u && address <= object + 0x68u &&
            (address & 3u) == 0u) {
            return 0x4000u + address - (object + 0x60u);
        }
        throw std::runtime_error("unexpected frame offset");
    }
    std::uint32_t RowCount(std::uint32_t list) override {
        Check(list == 0x2000u, "prescription list");
        calls.emplace_back("rows");
        return rows;
    }
    void SetHoverRow(std::uint32_t list,
                     std::uint32_t row) override {
        Check(list == 0x2000u, "hover list");
        hovered_row = row;
        calls.emplace_back("hover");
    }
    void SetImage(std::uint32_t window,
                  std::string_view imageset,
                  std::string_view name) override {
        Check(window == 0x3000u && imageset == "zi0",
              "mode image target");
        image = name;
        calls.emplace_back("image");
    }
    void SetSelected(std::uint32_t button,
                     bool value) override {
        Check(button >= 0x4000u && button <= 0x4008u &&
                  (button & 3u) == 0u,
              "mode button address");
        selected[(button - 0x4000u) / 4u] = value;
        calls.emplace_back("button");
    }
    void UpdateModelPreview(std::string_view group,
                       std::string_view key,
                       std::string_view path) override {
        Check(group == "zza" && path == "PALObject\\",
              "trade save arguments");
        save_key = key;
        calls.emplace_back("save");
    }
};

void CheckModes() {
    constexpr std::array<std::string_view, 3> images{
        "rongzhutupu", "duanyetupu", "zhulingtupu"};
    constexpr std::array<std::string_view, 3> keys{
        "ZZA1", "ZZA2", "ZZA3"};
    for (std::uint32_t mode = 0u; mode < 3u; ++mode) {
        Runtime runtime;
        runtime.mode = mode;
        Check(pal4::SmithWindow::OnCraftModeClick(
                  Runtime::object, 9u, runtime) == 1u &&
                  runtime.requested == 9u && runtime.hovered_row == 0u &&
                  runtime.image == images[mode] &&
                  runtime.save_key == keys[mode],
              "mode image, persistence and requested data");
        for (std::uint32_t index = 0u; index < 3u; ++index) {
            Check(runtime.selected[index] == (index == mode),
                  "exclusive mode button selection");
        }
        Check(runtime.calls == (std::vector<std::string>{
                  "clear", "load", "rows", "hover", "image",
                  "button", "button", "button", "save"}),
              "mode operation order");
    }
}

void CheckEmptyAndUnknown() {
    Runtime empty;
    empty.rows = 0u;
    Check(pal4::SmithWindow::OnCraftModeClick(
              Runtime::object, 0u, empty) == 1u &&
              empty.hovered_row == 0xffffffffu &&
              empty.image == "rongzhutupu",
          "empty list skips hover but still updates mode");
    Runtime unknown;
    unknown.mode = 3u;
    Check(pal4::SmithWindow::OnCraftModeClick(
              Runtime::object, 2u, unknown) == 1u &&
              unknown.image.empty() && unknown.save_key.empty() &&
              unknown.calls == (std::vector<std::string>{
                  "clear", "load", "rows", "hover"}),
          "unknown current mode does not change image or selection");
}

}  // namespace

int main() {
    try {
        CheckModes();
        CheckEmptyAndUnknown();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
