#include "pal4/magic_window_numeric_display.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(EXIT_FAILURE); }
}

struct Runtime final : pal4::MagicWindowNumericRuntimeStub {
    std::vector<std::string> calls;
    std::int32_t base{-12};
    std::int32_t remaining{5};
    std::int32_t total{42};

    std::int32_t QueryPlayerValue(std::uint32_t player,
                                  std::uint32_t role_id) override {
        Check(player == 0x9000u && role_id == 3u, "player virtual query inputs");
        calls.emplace_back("base");
        return base;
    }
    std::int32_t RemainingValue(std::uint32_t player,
                                 std::int32_t base_value) override {
        Check(player == 0x9000u && base_value == base,
              "remaining value uses player and virtual result");
        calls.emplace_back("remaining");
        return remaining;
    }
    std::int32_t TotalValue(std::uint32_t player,
                             std::uint32_t category_key) override {
        Check(player == 0x9000u && category_key == 2u,
              "total value uses current category key");
        calls.emplace_back("total");
        return total;
    }
    void SetText(std::uint32_t window, std::string_view text) override {
        calls.emplace_back("text:" + std::to_string(window) + ":" +
                           std::string(text));
    }
};

void TestValuesAndOrder() {
    pal4::MagicWindowPanelState panel;
    panel.At(0x90u) = 0x1000u;
    panel.At(0x94u) = 0x2000u;
    panel.At(0x98u) = 0x3000u;
    Runtime runtime;
    pal4::MagicWindowNumericDisplay::Update(0x9000u, 3u, 2u, panel, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "base", "text:4096:-12", "remaining", "text:8192:5",
              "total", "text:12288:42"},
          "virtual base, remaining and total display order");
}

void TestSignedFormatting() {
    pal4::MagicWindowPanelState panel;
    panel.At(0x90u) = 0x1000u;
    panel.At(0x94u) = 0x2000u;
    panel.At(0x98u) = 0x3000u;
    Runtime runtime;
    runtime.base = 0;
    runtime.remaining = -2147483647 - 1;
    runtime.total = 2147483647;
    pal4::MagicWindowNumericDisplay::Update(0x9000u, 3u, 2u, panel, runtime);
    Check(runtime.calls[1] == "text:4096:0" &&
              runtime.calls[3] == "text:8192:-2147483648" &&
              runtime.calls[5] == "text:12288:2147483647",
          "signed decimal formatting at int32 boundaries");
}

}  // namespace

int main() { TestValuesAndOrder(); TestSignedFormatting(); }
