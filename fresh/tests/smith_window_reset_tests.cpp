#include "pal4/smith_window.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowResetRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {object + 0x44u, 0x4400u},
        {object + 0x60u, 0x6000u},
        {object + 0x64u, 0x6400u},
        {object + 0x68u, 0x6800u},
    };
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, bool>> selected;
    std::uint32_t rows = 0u;

    void ClearSelection(std::uint32_t owner) override {
        Check(owner == object, "selection owner");
        calls.emplace_back("clear");
    }
    void LoadData(std::uint32_t owner,
                  std::uint32_t mode) override {
        Check(owner == object && mode == 0u, "data reload arguments");
        calls.emplace_back("load");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    std::uint32_t RowCount(std::uint32_t list) override {
        Check(list == 0x4400u, "prescription list");
        calls.emplace_back("row-count");
        return rows;
    }
    void SetHoverRow(std::uint32_t list,
                      std::uint32_t row) override {
        Check(list == 0x4400u && row == 0u,
              "first prescription row");
        calls.emplace_back("hover");
    }
    void SetSelected(std::uint32_t button,
                      bool value) override {
        selected.emplace_back(button, value);
        calls.emplace_back("selected");
    }
};

void CheckReset(const std::uint32_t rows) {
    Runtime runtime;
    runtime.rows = rows;
    pal4::SmithWindow::ResetState(Runtime::object, runtime);
    Check(runtime.selected ==
              (std::vector<std::pair<std::uint32_t, bool>>{
                  {0x6000u, false}, {0x6400u, false},
                  {0x6800u, false}}),
          "three Smith buttons are deselected in order");
    const std::vector<std::string> expected = rows == 0u
        ? std::vector<std::string>{
              "clear", "load", "read", "row-count",
              "read", "selected", "read", "selected",
              "read", "selected"}
        : std::vector<std::string>{
              "clear", "load", "read", "row-count", "hover",
              "read", "selected", "read", "selected",
              "read", "selected"};
    Check(runtime.calls == expected,
          "reset, list hover and button call order");
}

}  // namespace

int main() {
    try {
        CheckReset(0u);
        CheckReset(1u);
        CheckReset(7u);
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
