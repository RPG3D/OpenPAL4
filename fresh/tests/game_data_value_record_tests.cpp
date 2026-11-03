#include "pal4/game_data_value_record.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataValueRecordRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> reads;
    std::vector<std::uint32_t> writes;
    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        writes.push_back(address);
        words[address] = value;
    }
    void SetFloat(std::uint32_t offset, float value) {
        words[0x1000 + offset] = std::bit_cast<std::uint32_t>(value);
    }
};

Runtime MakeValid() {
    Runtime runtime;
    runtime.words[0x1008] = 0;
    runtime.words[0x100c] = 3;
    for (std::uint32_t offset = 24; offset <= 52; offset += 4)
        runtime.SetFloat(offset, 0.125f);
    runtime.SetFloat(56, 0.5f);
    runtime.SetFloat(60, 0.25f);
    runtime.SetFloat(64, 0.25f);
    return runtime;
}

void CheckClear() {
    Runtime runtime;
    for (std::uint32_t offset = 0; offset <= 72; offset += 4)
        runtime.words[0x1000 + offset] = 0xaaaaaaaau;
    Check(pal4::GameDataValueRecord::Clear(0x1000, runtime) == 0x1000 &&
              runtime.writes.size() == 13 &&
              runtime.writes.front() == 0x1010 &&
              runtime.writes.back() == 0x1040,
          "clear writes exactly thirteen consecutive DWORDs");
    for (std::uint32_t offset = 16; offset < 68; offset += 4)
        Check(runtime.words[0x1000 + offset] == 0,
              "clear zeroes every addressed DWORD");
    Check(runtime.words[0x100c] == 0xaaaaaaaau &&
              runtime.words[0x1044] == 0xaaaaaaaau,
          "clear preserves adjacent fields");
}

void CheckIndices() {
    for (std::int32_t index : {-1, 8}) {
        Runtime runtime = MakeValid();
        runtime.words[0x1008] = static_cast<std::uint32_t>(index);
        Check(!pal4::GameDataValueRecord::CheckValues(0x1000, runtime) &&
                  runtime.reads == std::vector<std::uint32_t>{0x1008},
              "signed first-index rejection reads nothing else");
    }
    for (std::int32_t index : {-1, 4}) {
        Runtime runtime = MakeValid();
        runtime.words[0x100c] = static_cast<std::uint32_t>(index);
        Check(!pal4::GameDataValueRecord::CheckValues(0x1000, runtime) &&
                  runtime.reads ==
                  std::vector<std::uint32_t>{0x1008, 0x100c},
              "signed second-index rejection skips floats");
    }
}

void CheckSums() {
    Runtime runtime = MakeValid();
    Check(pal4::GameDataValueRecord::CheckValues(0x1000, runtime),
          "two exact unit sums pass");
    Check(runtime.reads == std::vector<std::uint32_t>{
              0x1008, 0x100c, 0x1018, 0x101c, 0x1020, 0x1024,
              0x1028, 0x1030, 0x1034, 0x102c, 0x103c, 0x1040, 0x1038},
          "x87 additions use the observed nonmonotonic address order");

    runtime = MakeValid();
    runtime.SetFloat(24, 0.1265f);
    Check(!pal4::GameDataValueRecord::CheckValues(0x1000, runtime) &&
              runtime.reads.size() == 10,
          "first sum above unit plus epsilon rejects before tail reads");
    runtime = MakeValid();
    runtime.SetFloat(60, 0.252f);
    Check(!pal4::GameDataValueRecord::CheckValues(0x1000, runtime),
          "second sum above unit plus epsilon rejects");
    runtime = MakeValid();
    runtime.SetFloat(24, -2.0f);
    Check(pal4::GameDataValueRecord::CheckValues(0x1000, runtime),
          "negative sums have no lower bound");
    runtime = MakeValid();
    runtime.SetFloat(24, std::numeric_limits<float>::quiet_NaN());
    Check(pal4::GameDataValueRecord::CheckValues(0x1000, runtime),
          "unordered first comparison follows x87 masked branch");
    runtime = MakeValid();
    runtime.SetFloat(60, std::numeric_limits<float>::quiet_NaN());
    Check(pal4::GameDataValueRecord::CheckValues(0x1000, runtime),
          "unordered tail comparison follows x87 masked branch");
}

}  // namespace

int main() {
    try {
        CheckClear();
        CheckIndices();
        CheckSums();
        std::cout << "game_data_value_record_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
