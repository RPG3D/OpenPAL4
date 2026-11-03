#include "pal4/cegui_byte_buffer.h"

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

class Runtime final : public pal4::ICeguiByteBufferRuntime {
public:
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> freed;
    std::uint32_t allocated_bytes = 0u;

    std::uint32_t Allocate(const std::uint32_t count) override {
        allocated_bytes = count;
        return 0x5000u;
    }
    void Free(const std::uint32_t address) override { freed.push_back(address); }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
    }
};

void CheckBytes(Runtime& runtime, const std::uint32_t begin,
                const std::vector<std::uint8_t>& expected) {
    for (std::uint32_t index = 0; index < expected.size(); ++index) {
        Check(runtime.ReadByte(begin + index) == expected[index],
              "byte buffer content differs");
    }
}

void GrowthAndEmptyGrowth() {
    Runtime runtime;
    pal4::StaticCharacterBufferState state;
    state.multibyte_begin = 0x1000u;
    state.multibyte_end = state.multibyte_capacity_end = 0x1003u;
    runtime.bytes[0x1000u] = 'A';
    runtime.bytes[0x1001u] = 'B';
    runtime.bytes[0x1002u] = 'C';
    runtime.bytes[0x9000u] = 'X';
    Check(pal4::CeguiByteBuffer::Insert(
              state, 0x1001u, 2u, 0x9000u, runtime) == 0x5005u &&
              state.multibyte_begin == 0x5000u &&
              state.multibyte_capacity_end == 0x5006u &&
              runtime.allocated_bytes == 6u &&
              runtime.freed == std::vector<std::uint32_t>{0x1000u},
          "byte growth doubles old capacity");
    CheckBytes(runtime, 0x5000u, {'A', 'X', 'X', 'B', 'C'});

    Runtime empty_runtime;
    pal4::StaticCharacterBufferState empty;
    empty_runtime.bytes[0x9000u] = 'Q';
    Check(pal4::CeguiByteBuffer::Insert(
              empty, 0u, 2u, 0x9000u, empty_runtime) == 0x5002u &&
              empty.multibyte_capacity_end == 0x5002u &&
              empty_runtime.freed == std::vector<std::uint32_t>{0u},
          "empty byte growth forwards null free and allocates count");
    CheckBytes(empty_runtime, 0x5000u, {'Q', 'Q'});
}

void ShortTailAndAppendReturns() {
    Runtime runtime;
    pal4::StaticCharacterBufferState state;
    state.multibyte_begin = 0x1000u;
    state.multibyte_end = 0x1003u;
    state.multibyte_capacity_end = 0x100au;
    runtime.bytes[0x1000u] = 'A';
    runtime.bytes[0x1001u] = 'B';
    runtime.bytes[0x1002u] = 'C';
    runtime.bytes[0x9000u] = 'X';
    Check(pal4::CeguiByteBuffer::Insert(
              state, 0x1002u, 3u, 0x9000u, runtime) == 0x1003u &&
              state.multibyte_end == 0x1006u && runtime.freed.empty(),
          "short tail returns old end while new end advances");
    CheckBytes(runtime, 0x1000u, {'A', 'B', 'X', 'X', 'X', 'C'});
    Check(pal4::CeguiByteBuffer::Insert(
              state, 0x1006u, 2u, 0x9000u, runtime) == 0x1006u &&
              state.multibyte_end == 0x1008u,
          "append returns original insertion position");
    CheckBytes(runtime, 0x1000u, {'A', 'B', 'X', 'X', 'X', 'C', 'X', 'X'});
}

void LongTailAndZeroCount() {
    Runtime runtime;
    pal4::StaticCharacterBufferState state;
    state.multibyte_begin = 0x1000u;
    state.multibyte_end = 0x1004u;
    state.multibyte_capacity_end = 0x1008u;
    runtime.bytes[0x1000u] = 'A';
    runtime.bytes[0x1001u] = 'B';
    runtime.bytes[0x1002u] = 'C';
    runtime.bytes[0x1003u] = 'D';
    runtime.bytes[0x9000u] = 'X';
    Check(pal4::CeguiByteBuffer::Insert(
              state, 0x1001u, 2u, 0x9000u, runtime) == 0x1003u &&
              state.multibyte_end == 0x1006u,
          "long tail returns insertion position plus count");
    CheckBytes(runtime, 0x1000u, {'A', 'X', 'X', 'B', 'C', 'D'});
    Check(pal4::CeguiByteBuffer::Insert(
              state, 0x1002u, 0u, 0x9000u, runtime) == 0x1006u &&
              state.multibyte_end == 0x1006u,
          "zero-count byte insertion returns old end");
}

}  // namespace

int main() {
    GrowthAndEmptyGrowth();
    ShortTailAndAppendReturns();
    LongTailAndZeroCount();
    std::cout << "3 CEGUI byte-buffer test cases passed\n";
}
