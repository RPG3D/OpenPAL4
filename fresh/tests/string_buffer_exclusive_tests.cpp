#include "pal4/string_buffer.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::StringUniqueRuntimeStub {
public:
    std::vector<std::string> calls;
    std::vector<std::uint8_t> writes;
    std::uint8_t second_reference = 0xffu;
    bool change_second_reference = false;
    std::uint32_t reference_reads = 0u;

    void Seed(std::uint32_t address, std::uint8_t value) {
        bytes_[address] = value;
    }
    std::uint8_t Byte(std::uint32_t address) const {
        return bytes_.at(address);
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        calls.emplace_back("read");
        ++reference_reads;
        if (change_second_reference && reference_reads == 2u) {
            return second_reference;
        }
        return StringUniqueRuntimeStub::ReadByte(address);
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        calls.emplace_back("write");
        writes.push_back(value);
        StringUniqueRuntimeStub::WriteByte(address, value);
    }
    void ThrowLength() override {
        calls.emplace_back("throw_length");
        StringUniqueRuntimeStub::ThrowLength();
    }
    std::uint32_t SetSize(pal4::StringBufferState& state,
                          const std::uint32_t length) override {
        calls.emplace_back("set_size");
        Check(length == state.length, "set-size uses current length");
        return 0x12345667u;
    }
    std::uint32_t Reallocate(pal4::StringBufferState& state,
                             const std::uint32_t length) override {
        calls.emplace_back("reallocate");
        Check(length == state.length, "reallocate uses current length");
        state.data = 0x5001u;
        state.capacity = length;
        return 0x12345678u;
    }
};

void NullAndUnsharedPaths() {
    Runtime runtime;
    pal4::StringBufferState empty{};
    Check(pal4::StringBuffer::EnsureExclusive(empty, runtime) == 0u &&
              runtime.calls.empty(),
          "null data has no header operations");
    for (const auto reference : {0u, 0xffu}) {
        Runtime case_runtime;
        pal4::StringBufferState state{0x3001u, 4u, 4u};
        case_runtime.Seed(0x3000u,
                          static_cast<std::uint8_t>(reference));
        Check(pal4::StringBuffer::EnsureExclusive(
                  state, case_runtime) == reference &&
                  state.data == 0x3001u &&
                  case_runtime.Byte(0x3000u) == 0xffu &&
                  case_runtime.calls ==
                      std::vector<std::string>{"read", "write"},
              "zero and sentinel references only mark existing data exclusive");
    }
}

void SharedAndReallocatePaths() {
    Runtime runtime;
    runtime.Seed(0x3000u, 2u);
    pal4::StringBufferState empty_shared{0x3001u, 0u, 5u};
    Check(pal4::StringBuffer::EnsureExclusive(
              empty_shared, runtime) == 1u &&
              empty_shared.data == 0u && empty_shared.length == 0u &&
              empty_shared.capacity == 0u &&
              runtime.Byte(0x3000u) == 1u &&
              runtime.calls ==
                  std::vector<std::string>{"read", "read", "write"},
          "empty shared string decrements header and detaches all state");

    Runtime copying;
    copying.Seed(0x3000u, 3u);
    pal4::StringBufferState shared{0x3001u, 4u, 10u};
    Check(pal4::StringBuffer::EnsureExclusive(shared, copying) == 0x78u &&
              shared.data == 0x5001u && shared.capacity == 4u &&
              copying.Byte(0x5000u) == 0xffu &&
              copying.calls == std::vector<std::string>{
                  "read", "read", "reallocate", "write"},
          "nonempty shared string calls reallocate then marks new header");

    Runtime grown;
    grown.Seed(0x3000u, 2u);
    grown.change_second_reference = true;
    grown.second_reference = 0u;
    pal4::StringBufferState undersized{0x3001u, 8u, 4u};
    Check(pal4::StringBuffer::EnsureExclusive(
              undersized, grown) == 0x78u &&
              grown.calls == std::vector<std::string>{
                  "read", "read", "reallocate", "write"},
          "capacity below length reallocates after second header changes");
}

void ChangedHeaderAndLengthFailure() {
    Runtime runtime;
    runtime.Seed(0x3000u, 2u);
    runtime.change_second_reference = true;
    runtime.second_reference = 0u;
    pal4::StringBufferState state{0x3001u, 0u, 5u};
    Check(pal4::StringBuffer::EnsureExclusive(state, runtime) == 0x67u &&
              state.data == 0x3001u &&
              runtime.Byte(0x3000u) == 0xffu &&
              runtime.calls == std::vector<std::string>{
                  "read", "read", "set_size", "write"},
          "changed header with zero length uses size helper and marks header");

    Runtime oversized;
    oversized.Seed(0x3000u, 2u);
    pal4::StringBufferState too_long{0x3001u, 0xfffffffeu, 0u};
    bool caught = false;
    try {
        (void)pal4::StringBuffer::EnsureExclusive(too_long, oversized);
    } catch (const std::length_error&) {
        caught = true;
    }
    Check(caught && oversized.calls ==
              std::vector<std::string>{"read", "throw_length"} &&
              oversized.Byte(0x3000u) == 2u,
          "length above maximum throws before changing reference header");
}

}  // namespace

int main() {
    NullAndUnsharedPaths();
    SharedAndReallocatePaths();
    ChangedHeaderAndLengthFailure();
    std::cout << "3 string buffer exclusive test cases passed\n";
}
