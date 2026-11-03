#include "pal4/cegui_wide_buffer.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::ICeguiWideBufferRuntime {
public:
    std::map<std::uint32_t, std::uint16_t> words;
    std::vector<std::uint32_t> freed;
    std::uint32_t allocated_bytes = 0u;
    std::uint32_t next_address = 0x5000u;

    std::uint32_t Allocate(const std::uint32_t bytes) override {
        allocated_bytes = bytes;
        return next_address;
    }
    void Free(const std::uint32_t address) override { freed.push_back(address); }
    std::uint16_t ReadWord(const std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint16_t value) override {
        words[address] = value;
    }
};

void CheckWords(Runtime& runtime, const std::uint32_t begin,
                const std::vector<std::uint16_t>& expected) {
    for (std::uint32_t index = 0; index < expected.size(); ++index) {
        Check(runtime.ReadWord(begin + 2u * index) == expected[index],
              "wide buffer content differs");
    }
}

void GrowthCopiesPrefixFillAndSuffix() {
    Runtime runtime;
    pal4::StaticCharacterBufferState state;
    state.wide_begin = 0x1000u;
    state.wide_end = state.wide_capacity_end = 0x1006u;
    runtime.words[0x1000u] = 'A';
    runtime.words[0x1002u] = 'B';
    runtime.words[0x1004u] = 'C';
    runtime.words[0x9000u] = 'X';
    const auto result = pal4::CeguiWideBuffer::Insert(
        state, 0x1002u, 2u, 0x9000u, runtime);
    Check(result == 0x500au && state.wide_begin == 0x5000u &&
              state.wide_end == 0x500au && state.wide_capacity_end == 0x500cu &&
              runtime.allocated_bytes == 12u &&
              runtime.freed == std::vector<std::uint32_t>{0x1000u},
          "growth doubles old capacity and releases old storage");
    CheckWords(runtime, 0x5000u, {'A', 'X', 'X', 'B', 'C'});
}

void EmptyGrowthUsesRequestedCount() {
    Runtime runtime;
    pal4::StaticCharacterBufferState state;
    runtime.words[0x9000u] = 'Q';
    Check(pal4::CeguiWideBuffer::Insert(
              state, 0u, 3u, 0x9000u, runtime) == 0x5006u &&
              state.wide_begin == 0x5000u &&
              state.wide_capacity_end == 0x5006u &&
              runtime.allocated_bytes == 6u &&
              runtime.freed == std::vector<std::uint32_t>{0u},
          "empty growth allocates count words and forwards null to free");
    CheckWords(runtime, 0x5000u, {'Q', 'Q', 'Q'});
}

void InsertWithShortTail() {
    Runtime runtime;
    pal4::StaticCharacterBufferState state;
    state.wide_begin = 0x1000u;
    state.wide_end = 0x1006u;
    state.wide_capacity_end = 0x1010u;
    runtime.words[0x1000u] = 'A';
    runtime.words[0x1002u] = 'B';
    runtime.words[0x1004u] = 'C';
    runtime.words[0x9000u] = 'X';
    Check(pal4::CeguiWideBuffer::Insert(
              state, 0x1004u, 3u, 0x9000u, runtime) == 0x100cu &&
              state.wide_begin == 0x1000u && runtime.freed.empty() &&
              runtime.allocated_bytes == 0u,
          "short-tail insertion stays in existing storage");
    CheckWords(runtime, 0x1000u, {'A', 'B', 'X', 'X', 'X', 'C'});
}

void InsertWithLongTailAndZeroCount() {
    Runtime runtime;
    pal4::StaticCharacterBufferState state;
    state.wide_begin = 0x1000u;
    state.wide_end = 0x1008u;
    state.wide_capacity_end = 0x1010u;
    runtime.words[0x1000u] = 'A';
    runtime.words[0x1002u] = 'B';
    runtime.words[0x1004u] = 'C';
    runtime.words[0x1006u] = 'D';
    runtime.words[0x9000u] = 'X';
    Check(pal4::CeguiWideBuffer::Insert(
              state, 0x1002u, 2u, 0x9000u, runtime) == 0x1006u &&
              state.wide_end == 0x100cu,
          "long-tail insertion returns inserted-range end and advances buffer end");
    CheckWords(runtime, 0x1000u, {'A', 'X', 'X', 'B', 'C', 'D'});
    Check(pal4::CeguiWideBuffer::Insert(
              state, 0x1004u, 0u, 0x9000u, runtime) == 0x100cu &&
              state.wide_end == 0x100cu && runtime.freed.empty(),
          "zero-count insertion preserves buffer");
}

void CopyForwardAndEmptyRange() {
    Runtime runtime;
    pal4::StaticCharacterBufferState state;
    state.wide_begin = 0x1000u;
    state.wide_end = 0x1008u;
    state.wide_capacity_end = 0x1010u;
    runtime.words[0x1000u] = 'A';
    runtime.words[0x1002u] = 'B';
    runtime.words[0x1004u] = 'C';
    runtime.words[0x1006u] = 'D';
    Check(pal4::CeguiWideBuffer::CopyTo(
              state, 0x1000u, 0x1004u, runtime) == 0x1000u &&
              state.wide_end == 0x1004u,
          "copy returns destination and sets end to copied range");
    CheckWords(runtime, 0x1000u, {'C', 'D'});
    Check(pal4::CeguiWideBuffer::CopyTo(
              state, 0x1006u, 0x1004u, runtime) == 0x1006u &&
              state.wide_end == 0x1006u,
          "empty copy sets end to destination without copying");
}

}  // namespace

int main() {
    GrowthCopiesPrefixFillAndSuffix();
    EmptyGrowthUsesRequestedCount();
    InsertWithShortTail();
    InsertWithLongTailAndZeroCount();
    CopyForwardAndEmptyRange();
    std::cout << "5 CEGUI wide-buffer test cases passed\n";
}
