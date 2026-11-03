#include "pal4/game_data_progress_ratio.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataProgressRatioRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> reads;
    std::uint32_t GameDatabase() override { return 0x1000u; }
    std::uint32_t GlobalData() override { return 0x3000u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return words[address];
    }
};

void CheckRatios() {
    Runtime runtime;
    runtime.words[0x1060u] = 0x2000u;
    runtime.words[0x2060u] = 0x4000u;
    runtime.words[0x2064u] = 0x44b0u;
    runtime.words[0x30b0u] = 7u;
    Check(std::abs(pal4::GameDataProgressRatio::Calculate(runtime) -
                   7.0 / 3.0) < 1e-12 &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x1060u, 0x2060u, 0x2064u, 0x30b0u},
          "three 400-byte records divide unsigned global count");

    runtime.words[0x30b0u] = 0xffffffffu;
    Check(std::abs(pal4::GameDataProgressRatio::Calculate(runtime) -
                   4294967295.0 / 3.0) < 1e-6,
          "global DWORD uses unsigned conversion");

    runtime.words[0x2060u] = 0u;
    runtime.words[0x30b0u] = 5u;
    Check(std::isinf(pal4::GameDataProgressRatio::Calculate(runtime)),
          "empty record array retains floating zero denominator");
    runtime.words[0x30b0u] = 0u;
    Check(std::isnan(pal4::GameDataProgressRatio::Calculate(runtime)),
          "zero over zero retains floating NaN");

    runtime.words[0x2060u] = 0x4000u;
    runtime.words[0x2064u] = 0x4000u - 400u;
    runtime.words[0x30b0u] = 2u;
    Check(pal4::GameDataProgressRatio::Calculate(runtime) == -2.0,
          "record-span division is signed");
}

void CheckItemRatios() {
    Runtime runtime;
    runtime.words[0x1004u] = 0x2000u;
    runtime.words[0x2060u] = 0x4000u;
    runtime.words[0x2064u] = 0x41f8u;
    runtime.words[0x30a0u] = 5u;
    Check(std::abs(pal4::GameDataProgressRatio::CalculateItems(runtime) -
                   5.0 / 3.0) < 1e-12 &&
              runtime.reads == std::vector<std::uint32_t>{
                  0x1004u, 0x2060u, 0x2064u, 0x30a0u},
          "three 168-byte item records divide unsigned global count");
    runtime.words[0x2060u] = 0u;
    Check(std::isinf(pal4::GameDataProgressRatio::CalculateItems(runtime)),
          "item ratio preserves infinity on an empty collection");
    runtime.words[0x30a0u] = 0u;
    Check(std::isnan(pal4::GameDataProgressRatio::CalculateItems(runtime)),
          "item ratio preserves zero-over-zero NaN");
    runtime.words[0x2060u] = 0x4000u;
    runtime.words[0x2064u] = 0x4000u - 168u;
    runtime.words[0x30a0u] = 2u;
    Check(pal4::GameDataProgressRatio::CalculateItems(runtime) == -2.0,
          "item record span uses signed division");
}

}  // namespace

int main() {
    try {
        CheckRatios();
        CheckItemRatios();
        std::cout << "game_data_progress_ratio_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
