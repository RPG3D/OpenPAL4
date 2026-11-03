#include "pal4/static_character_buffers.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IStaticCharacterBufferRuntime {
public:
    std::uint32_t wide_allocation = 0x1000u;
    std::uint32_t multibyte_allocation = 0x2000u;
    int register_result = 37;
    std::map<std::uint32_t, std::uint16_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    std::vector<std::uint32_t> callbacks;
    std::vector<std::string> events;
    pal4::StaticCharacterBufferState* state = nullptr;

    std::uint32_t Allocate(std::uint32_t size) override {
        allocations.push_back(size);
        events.emplace_back("allocate");
        return size == 0x100u ? wide_allocation : multibyte_allocation;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void WriteWord(std::uint32_t address,
                   std::uint16_t value) override {
        words[address] = value;
    }
    void Free(std::uint32_t address) override {
        Check(state != nullptr, "state is available during free");
        Check((state->cleanup_bits & (address == state->wide_begin ? 1u : 2u))
                  != 0u,
              "cleanup bit is published before free");
        frees.push_back(address);
        events.emplace_back("free");
    }
    int RegisterExit(std::uint32_t callback) override {
        callbacks.push_back(callback);
        events.emplace_back("register");
        return register_result;
    }
};

void WideInitializationAndCleanup() {
    pal4::StaticCharacterBufferState state;
    Runtime runtime;
    runtime.state = &state;
    pal4::StaticCharacterBuffers buffers(state);
    Check(buffers.InitializeWideAndRegister(0xa5u, runtime) == 37 &&
              state.wide_header_byte == 0xa5u &&
              state.wide_begin == 0x1000u &&
              state.wide_end == 0x1100u &&
              state.wide_capacity_end == 0x1100u &&
              runtime.allocations == std::vector<std::uint32_t>{0x100u} &&
              runtime.callbacks == std::vector<std::uint32_t>{0x410360u} &&
              runtime.events == std::vector<std::string>{
                  "allocate", "register"},
          "wide initializer records layout and registers cleanup");
    Check(runtime.words.size() == 128u &&
              runtime.words.at(0x1000u) == 0u &&
              runtime.words.at(0x10feu) == 0u,
          "wide initializer clears exactly 128 words");
    state.cleanup_bits = 0x80u;
    buffers.CleanupWide(runtime);
    Check(state.cleanup_bits == 0x81u && state.wide_begin == 0u &&
              state.wide_end == 0u && state.wide_capacity_end == 0u &&
              runtime.frees == std::vector<std::uint32_t>{0x1000u},
          "wide cleanup sets its guard and clears all pointers");
    buffers.CleanupWide(runtime);
    Check(runtime.frees.size() == 1u,
          "second wide cleanup is guarded");
}

void MultibyteInitializationAndIndependentCleanupBit() {
    pal4::StaticCharacterBufferState state;
    Runtime runtime;
    runtime.state = &state;
    pal4::StaticCharacterBuffers buffers(state);
    state.cleanup_bits = 0x81u;
    Check(buffers.InitializeMultibyteAndRegister(0x6au, runtime) == 37 &&
              state.multibyte_header_byte == 0x6au &&
              state.multibyte_begin == 0x2000u &&
              state.multibyte_end == 0x2080u &&
              state.multibyte_capacity_end == 0x2080u &&
              runtime.callbacks == std::vector<std::uint32_t>{0x410410u},
          "multibyte initializer stores its own buffer and callback");
    Check(runtime.bytes.size() == 128u &&
              runtime.bytes.at(0x2000u) == 0u &&
              runtime.bytes.at(0x207fu) == 0u,
          "multibyte initializer clears exactly 128 bytes");
    buffers.CleanupMultibyte(runtime);
    Check(state.cleanup_bits == 0x83u &&
              state.multibyte_begin == 0u &&
              state.multibyte_end == 0u &&
              state.multibyte_capacity_end == 0u &&
              runtime.frees == std::vector<std::uint32_t>{0x2000u},
          "multibyte cleanup preserves wide and other guard bits");
    buffers.CleanupMultibyte(runtime);
    Check(runtime.frees.size() == 1u,
          "second multibyte cleanup is guarded");
}

void FailedAllocationsRetainObservedEndArithmetic() {
    pal4::StaticCharacterBufferState state;
    Runtime runtime;
    runtime.state = &state;
    runtime.wide_allocation = 0u;
    runtime.multibyte_allocation = 0u;
    pal4::StaticCharacterBuffers buffers(state);
    Check(buffers.InitializeWide(0x11u, runtime) == 0x100u &&
              state.wide_begin == 0u && runtime.words.size() == 127u &&
              runtime.words.at(2u) == 0u &&
              runtime.words.at(0xfeu) == 0u,
          "null wide allocation still attempts subsequent low-address writes");
    Check(buffers.InitializeMultibyte(0x22u, runtime) == 0x80u &&
              state.multibyte_begin == 0u && runtime.bytes.size() == 127u &&
              runtime.bytes.at(1u) == 0u &&
              runtime.bytes.at(0x7fu) == 0u,
          "null multibyte allocation also attempts low-address writes");
    buffers.CleanupWide(runtime);
    buffers.CleanupMultibyte(runtime);
    Check(runtime.frees == std::vector<std::uint32_t>{0u, 0u} &&
              state.cleanup_bits == 3u,
          "both cleanup callbacks still call free with null pointers");
}

}  // namespace

int main() {
    WideInitializationAndCleanup();
    MultibyteInitializationAndIndependentCleanupBit();
    FailedAllocationsRetainObservedEndArithmetic();
    std::cout << "3 static character buffer test cases passed\n";
}
