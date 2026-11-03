#include "pal4/jigsaw_window_storage.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Span {
    std::uint32_t first;
    std::uint32_t second;
    bool operator==(const Span&) const = default;
};

struct SizedFree {
    std::uint32_t address;
    std::int32_t words;
    bool operator==(const SizedFree&) const = default;
};

struct Runtime final : pal4::IJigsawWindowStorageRuntime {
    static constexpr std::uint32_t puzzle = 0x1000u;
    static constexpr std::uint32_t frame = puzzle - 0x2cu;
    static constexpr std::uint32_t board = 0x3000u;
    static constexpr std::uint32_t groups = 0x4000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> strings;
    std::vector<Span> cells;
    std::vector<Span> ranges;
    std::vector<std::uint32_t> raw_frees;
    std::vector<std::uint32_t> zero_frees;
    std::vector<SizedFree> sized_frees;
    std::vector<std::string> calls;
    std::uint32_t noops = 0u;
    std::uint32_t destroyed = 0u;

    Runtime() {
        words[puzzle + 0x1cu] = board;
        words[puzzle + 0x20u] = board + 0x190u;
        words[puzzle + 0x24u] = board + 0x258u;
        words[puzzle + 4u] = groups;
        words[puzzle + 8u] = groups + 0x20u;
        words[puzzle + 0xcu] = groups + 0x30u;
        words[groups + 4u] = 0x5000u;
        words[groups + 8u] = 0x5008u;
        words[groups + 0xcu] = 0x5010u;
        words[groups + 0x14u] = 0x6000u;
        words[groups + 0x18u] = 0x6004u;
        words[groups + 0x1cu] = 0x600cu;
    }

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write");
    }
    void DestroyString(std::uint32_t address) override {
        strings.push_back(address);
        calls.emplace_back("string");
    }
    void TouchCell(std::uint32_t group,
                   std::uint32_t cell) override {
        cells.push_back({group, cell});
        calls.emplace_back("cell");
    }
    void TouchRange(std::uint32_t begin,
                    std::uint32_t end) override {
        ranges.push_back({begin, end});
        calls.emplace_back("range");
    }
    void TouchNoop() override {
        ++noops;
        calls.emplace_back("noop");
    }
    void FreeRaw(std::uint32_t address) override {
        raw_frees.push_back(address);
        calls.emplace_back("raw-free");
    }
    std::uint32_t FreeZero(std::uint32_t address) override {
        zero_frees.push_back(address);
        calls.emplace_back("zero-free");
        return 0xa500u + static_cast<std::uint32_t>(zero_frees.size());
    }
    void FreeSized(std::uint32_t address,
                   std::int32_t count) override {
        sized_frees.push_back({address, count});
        calls.emplace_back("sized-free");
    }
    std::uint32_t DestroyBase(std::uint32_t address) override {
        destroyed = address;
        calls.emplace_back("base");
        return 0xbeefu;
    }
};

void CheckClearedWords(const Runtime& runtime) {
    for (const auto address : {
             Runtime::puzzle + 0x1cu, Runtime::puzzle + 0x20u,
             Runtime::puzzle + 0x24u, Runtime::puzzle + 4u,
             Runtime::puzzle + 8u, Runtime::puzzle + 0xcu,
             Runtime::groups + 4u, Runtime::groups + 8u,
             Runtime::groups + 0xcu, Runtime::groups + 0x14u,
             Runtime::groups + 0x18u, Runtime::groups + 0x1cu})
        Check(runtime.words.at(address) == 0u, "all vector words cleared");
}

void CheckCleanup() {
    Runtime runtime;
    Check(pal4::JigsawWindowStorage::Cleanup(
              Runtime::puzzle, runtime) == 0xa502u,
          "cleanup returns outer FreeZero result");
    Check(runtime.strings == (std::vector<std::uint32_t>{
              Runtime::puzzle + 0x98u, Runtime::board + 0x10u,
              Runtime::board + 0xd8u}) &&
              runtime.cells == (std::vector<Span>{
                  {Runtime::groups, 0x5000u},
                  {Runtime::groups, 0x5004u},
                  {Runtime::groups + 0x10u, 0x6000u}}) &&
              runtime.raw_frees ==
                  (std::vector<std::uint32_t>{0x5000u, 0x6000u}) &&
              runtime.zero_frees == (std::vector<std::uint32_t>{
                  Runtime::board, Runtime::groups}) &&
              runtime.sized_frees.empty(),
          "cleanup uses 0xc8 board and 0x10 group strides");
    CheckClearedWords(runtime);
    Check(runtime.calls.front() == "string" &&
              runtime.calls.back() == "write",
          "cleanup destroys text before clearing array words");
}

void CheckDestruct() {
    Runtime runtime;
    Check(pal4::JigsawWindowStorage::Destruct(
              Runtime::frame, runtime) == 0xbeefu &&
              runtime.destroyed == Runtime::frame,
          "frame destruction returns base result");
    Check(runtime.ranges == (std::vector<Span>{
              {0x5000u, 0x5008u}, {0x6000u, 0x6004u}}) &&
              runtime.sized_frees == (std::vector<SizedFree>{
                  {0x5000u, 4}, {0x6000u, 3}}) &&
              runtime.raw_frees.empty() && runtime.cells.empty() &&
              runtime.zero_frees == (std::vector<std::uint32_t>{
                  Runtime::board, Runtime::groups}),
          "destructor uses range callback and capacity-based free");
    CheckClearedWords(runtime);
    Check(runtime.calls.back() == "base",
          "base destructor runs after vector cleanup");
}

void CheckGroupClear() {
    Runtime runtime;
    Check(pal4::JigsawWindowStorage::ClearGroups(
              Runtime::puzzle, runtime) == 0u &&
              runtime.noops == 3u &&
              runtime.zero_frees == (std::vector<std::uint32_t>{
                  0x5000u, 0x6000u, Runtime::groups}) &&
              runtime.strings.empty() && runtime.sized_frees.empty(),
          "group clearer calls no-op once per cell and frees three arrays");
    for (const auto address : {Runtime::puzzle + 4u,
                               Runtime::puzzle + 8u,
                               Runtime::puzzle + 0xcu})
        Check(runtime.words.at(address) == 0u, "outer group vector reset");
}

void CheckEmpty() {
    Runtime runtime;
    runtime.words[Runtime::puzzle + 0x1cu] = 0u;
    runtime.words[Runtime::puzzle + 0x20u] = 0u;
    runtime.words[Runtime::puzzle + 4u] = 0u;
    runtime.words[Runtime::puzzle + 8u] = 0u;
    Check(pal4::JigsawWindowStorage::Cleanup(
              Runtime::puzzle, runtime) == 0xa502u &&
              runtime.strings == (std::vector<std::uint32_t>{
                  Runtime::puzzle + 0x98u}) &&
              runtime.zero_frees ==
                  (std::vector<std::uint32_t>{0u, 0u}) &&
              runtime.raw_frees.empty(),
          "empty vectors still pass null to FreeZero");
}

}  // namespace

int main() {
    try {
        CheckCleanup();
        CheckDestruct();
        CheckGroupClear();
        CheckEmpty();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
