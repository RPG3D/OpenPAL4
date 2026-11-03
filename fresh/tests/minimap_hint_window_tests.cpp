#include "pal4/minimap_hint_window.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IMinimapHintWindowRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::string layout;
    std::string copied;
    std::uint32_t destination = 0u;
    std::uint32_t error_record = 0u;
    std::uint8_t error_flag = 0u;
    std::uint32_t reallocate_record = 0u;
    std::uint32_t reallocate_length = 0u;
    std::uint32_t initial_data = 0x9000u;
    std::uint32_t initial_capacity = 31u;
    std::uint8_t initial_reference = 0u;

    void LoadLayout(std::uint32_t window,
                    std::string_view layout_name) override {
        Check(window == 0x1000u, "base constructor owner");
        calls.emplace_back("base");
        layout = layout_name;
        words[window + 52u] = initial_data;
        words[window + 56u] = 15u;
        words[window + 60u] = initial_capacity;
        words[window + 36u] = 0x8000u;
        if (initial_data != 0u)
            bytes[initial_data - 1u] = initial_reference;
    }
    std::uint32_t DestroyBase(std::uint32_t) override { return 0u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        if (address == 0x1000u &&
            value == pal4::MinimapHintWindow::kVtable)
            calls.emplace_back("vtable");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        if (address != 0x1040u) calls.emplace_back("nul");
    }
    void SetErrorFlag(std::uint32_t record,
                      std::uint8_t flag) override {
        if (flag == 0u) return;
        calls.emplace_back("error");
        error_record = record;
        error_flag = flag;
    }
    void Reallocate(std::uint32_t record,
                    std::uint32_t length) override {
        calls.emplace_back("reallocate");
        reallocate_record = record;
        reallocate_length = length;
        words[0x1034u] = 0xa000u;
        words[0x103cu] = length;
    }
    void CopyText(std::uint32_t address,
                  std::string_view text) override {
        if (text == pal4::HintWindow::kClockName) return;
        calls.emplace_back("copy");
        destination = address;
        copied = text;
    }
    bool AllocateText(std::uint32_t, std::uint32_t,
                      std::uint8_t) override { return true; }
    void SetTextSize(std::uint32_t, std::uint32_t) override {}
    std::string GetNamePrefix(std::uint32_t field) override {
        Check(field == 0x1018u, "name prefix field");
        return layout;
    }
    std::uint32_t FindWindow(std::string_view) override {
        return 0x8200u;
    }
    void SetAutoBreak(std::uint32_t, bool) override {}
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 12u, "z-order callback size");
        return 0u;
    }
    std::uint32_t Subscribe(std::uint32_t, std::uint32_t,
                            std::uint32_t) override { return 0u; }
    void ReleaseConnection(std::uint32_t) override {}
    void Free(std::uint32_t) override {}
};

void CheckUniqueStorage() {
    Runtime runtime;
    const auto window = pal4::MinimapHintWindow::Construct(0x1000u, runtime);
    Check(window == 0x1000u &&
              runtime.layout == pal4::MinimapHintWindow::kLayout &&
              runtime.words.at(0x1000u) ==
                  pal4::MinimapHintWindow::kVtable &&
              runtime.copied == "MinimapHintWindowClock" &&
              runtime.destination == 0x9000u &&
              runtime.words.at(0x1038u) == 22u &&
              runtime.bytes.at(0x9000u + 22u) == 0u &&
              runtime.calls == (std::vector<std::string>{
                  "base", "vtable", "copy", "nul"}),
          "unique short string storage is reused for the clock name");
}

void CheckReallocationBranches() {
    for (const auto reference : {1u, 0xfeu}) {
        Runtime runtime;
        runtime.initial_reference = static_cast<std::uint8_t>(reference);
        (void)pal4::MinimapHintWindow::Construct(0x1000u, runtime);
        Check(runtime.reallocate_record == 0x1030u &&
                  runtime.reallocate_length == 22u &&
                  runtime.destination == 0xa000u &&
                  runtime.error_record == 0u &&
                  runtime.calls == (std::vector<std::string>{
                      "base", "vtable", "reallocate", "copy", "nul"}),
              "shared reference detaches before writing the new clock name");
    }
    for (const auto capacity : {10u, 32u}) {
        Runtime runtime;
        runtime.initial_capacity = capacity;
        (void)pal4::MinimapHintWindow::Construct(0x1000u, runtime);
        Check(runtime.error_record == 0x1030u &&
                  runtime.error_flag == 1u &&
                  runtime.reallocate_record == 0x1030u &&
                  runtime.destination == 0xa000u &&
                  runtime.calls == (std::vector<std::string>{
                      "base", "vtable", "error", "reallocate",
                      "copy", "nul"}),
              "insufficient or heap capacity triggers reset and reallocation");
    }
    Runtime no_data;
    no_data.initial_data = 0u;
    no_data.initial_capacity = 0u;
    (void)pal4::MinimapHintWindow::Construct(0x1000u, no_data);
    Check(no_data.error_record == 0x1030u &&
              no_data.reallocate_record == 0x1030u &&
              no_data.destination == 0xa000u,
          "empty data allocates storage for the clock name");
}

}  // namespace

int main() {
    try {
        CheckUniqueStorage();
        CheckReallocationBranches();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
