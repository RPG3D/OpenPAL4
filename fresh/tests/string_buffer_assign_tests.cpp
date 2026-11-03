#include "pal4/string_buffer.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IStringAssignRuntime {
public:
    void ThrowRange() override {
        events.emplace_back("range");
        throw std::out_of_range("range");
    }
    void ThrowLength() override {
        events.emplace_back("length");
        throw std::length_error("length");
    }
    void DetachAndReset(pal4::StringBufferState&) override {
        events.emplace_back("detach");
    }
    bool AllocateText(pal4::StringBufferState&, std::uint32_t length,
                      std::uint8_t mode) override {
        Check(mode == 0u, "self assignment uses allocation mode zero");
        events.emplace_back("allocate:" + std::to_string(length));
        return allocate_succeeds;
    }
    void SetTextSize(pal4::StringBufferState& state,
                     std::uint32_t length) override {
        events.emplace_back("size");
        state.length = length;
        WriteByte(state.data + length, 0u);
    }
    void SetErrorFlag(pal4::StringBufferState&, std::uint8_t flag) override {
        events.emplace_back("error:" + std::to_string(flag));
    }
    void Reallocate(pal4::StringBufferState& state,
                    std::uint32_t length) override {
        length_before_reallocate = state.length;
        events.emplace_back("reallocate:" + std::to_string(length));
        state.data = 0x5001u;
        state.capacity = length + 8u;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        events.emplace_back("write:" + std::to_string(address));
        bytes[address] = value;
    }
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t length) override {
        events.emplace_back("copy:" + std::to_string(length));
        std::vector<std::uint8_t> data;
        for (std::uint32_t i = 0u; i < length; ++i) {
            data.push_back(bytes.at(source + i));
        }
        for (std::uint32_t i = 0u; i < length; ++i) {
            bytes[destination + i] = data[i];
        }
    }
    void Free(std::uint32_t address) override {
        events.emplace_back("free:" + std::to_string(address));
    }

    void Put(std::uint32_t address, const std::string& text) {
        for (std::size_t i = 0u; i < text.size(); ++i) {
            bytes[address + static_cast<std::uint32_t>(i)] =
                static_cast<std::uint8_t>(text[i]);
        }
        bytes[address + static_cast<std::uint32_t>(text.size())] = 0u;
    }

    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::uint32_t length_before_reallocate{};
    bool allocate_succeeds = true;
};

class ResetRuntime final : public pal4::IStringResetRuntime {
public:
    std::uint8_t ReadByte(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        events.emplace_back("write:" + std::to_string(address));
        bytes[address] = value;
    }
    std::uint32_t Free(std::uint32_t address) override {
        events.emplace_back("free:" + std::to_string(address));
        return 0x7bu;
    }
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
};

void CheckRangeAndLengthErrors() {
    pal4::StringBuffer destination({0x1001u, 1u, 8u});
    pal4::StringBuffer source({0x2001u, 3u, 8u});
    Runtime runtime;
    bool range = false;
    try {
        (void)destination.Assign(source, 4u, 1u, runtime);
    } catch (const std::out_of_range&) {
        range = true;
    }
    Check(range && runtime.events == std::vector<std::string>{"range"},
          "start after source length raises range before memory access");

    runtime.events.clear();
    runtime.bytes[0x2000u] = 0xfeu;
    pal4::StringBuffer huge_source({0x2001u, 0xfffffffeu, 8u});
    bool length = false;
    try {
        (void)destination.Assign(huge_source, 0u, 0xfffffffeu, runtime);
    } catch (const std::length_error&) {
        length = true;
    }
    Check(length && runtime.events.back() == "length" &&
              destination.State().data == 0x1001u,
          "oversized selected range raises length before destination change");
}

void CheckSharedWholeSource() {
    pal4::StringBuffer destination({0x1001u, 2u, 8u});
    pal4::StringBuffer source({0x2001u, 3u, 12u});
    Runtime runtime;
    runtime.bytes[0x1000u] = 0u;
    runtime.bytes[0x2000u] = 2u;
    Check(&destination.Assign(source, 0u, 99u, runtime) == &destination &&
              destination.State().data == 0x2001u &&
              destination.State().length == 3u &&
              destination.State().capacity == 12u &&
              runtime.bytes[0x2000u] == 3u &&
              runtime.events == std::vector<std::string>{
                  "read:8192", "read:4096", "free:4096",
                  "read:8192", "write:8192"},
          "whole source shares reference, frees old header, and increments count");
}

void CheckSubstringCopyAndReallocation() {
    pal4::StringBuffer destination({0x1001u, 2u, 40u});
    pal4::StringBuffer source({0x2001u, 5u, 8u});
    Runtime runtime;
    runtime.bytes[0x1000u] = 3u;
    runtime.Put(0x2001u, "abcde");
    Check(&destination.Assign(source, 1u, 3u, runtime) == &destination &&
              destination.State().data == 0x5001u &&
              destination.State().length == 3u &&
              destination.State().capacity == 11u &&
              runtime.bytes[0x5001u] == 'b' &&
              runtime.bytes[0x5002u] == 'c' &&
              runtime.bytes[0x5003u] == 'd' &&
              runtime.bytes[0x5004u] == 0u,
          "substring from shared destination reallocates and copies selected bytes");
    Check(runtime.events[0] == "read:4096" &&
              runtime.events[1] == "reallocate:3",
          "reference check precedes reallocation");
}

void CheckEmptySelectionBranches() {
    pal4::StringBuffer source({0x2001u, 0u, 8u});
    Runtime runtime;
    pal4::StringBuffer shared({0x1001u, 4u, 8u});
    runtime.bytes[0x1000u] = 3u;
    Check(&shared.Assign(source, 0u, 0u, runtime) == &shared &&
              runtime.bytes[0x1000u] == 2u &&
              runtime.events.back() == "error:0" &&
              shared.State().length == 4u,
          "empty assignment on shared storage decrements reference only");

    runtime.events.clear();
    runtime.bytes[0x1000u] = 0xffu;
    pal4::StringBuffer unique({0x1001u, 4u, 8u});
    Check(&unique.Assign(source, 0u, 0u, runtime) == &unique &&
              runtime.bytes[0x1000u] == 0xffu &&
              runtime.events.back() == "error:1",
          "empty assignment on unique storage only sets error flag");
}

void CheckSelfSubstring() {
    pal4::StringBuffer buffer({0x2001u, 6u, 12u});
    Runtime runtime;
    runtime.Put(0x2001u, "abcdef");
    Check(&buffer.Assign(buffer, 2u, 2u, runtime) == &buffer &&
              buffer.State().length == 2u &&
              runtime.bytes[0x2001u] == 'c' &&
              runtime.bytes[0x2002u] == 'd' &&
              runtime.bytes[0x2003u] == 0u &&
              runtime.events == std::vector<std::string>{
                  "detach", "copy:0", "allocate:4", "write:8197",
                  "detach", "copy:2", "allocate:2", "size",
                  "write:8195"},
          "self substring performs two detach stages and retains selected bytes");
}

void CheckUnshareableSourceUsesExistingCapacity() {
    pal4::StringBuffer destination({0x1001u, 2u, 8u});
    pal4::StringBuffer source({0x2001u, 3u, 8u});
    Runtime runtime;
    runtime.bytes[0x2000u] = 0xfeu;
    runtime.bytes[0x1000u] = 0u;
    runtime.Put(0x2001u, "xyz");
    Check(&destination.Assign(source, 0u, 3u, runtime) == &destination &&
              destination.State().data == 0x1001u &&
              destination.State().length == 3u &&
              runtime.bytes[0x1001u] == 'x' &&
              runtime.bytes[0x1004u] == 0u,
          "source reference fe bypasses sharing and reuses small destination");
    for (const auto& event : runtime.events) {
        Check(event.find("reallocate") == std::string::npos,
              "small capacity path does not reallocate");
    }
}

void CheckSetErrorFlag() {
    ResetRuntime runtime;
    pal4::StringBuffer no_release({0x1201u, 8u, 16u});
    Check(no_release.SetErrorFlag(0u, runtime) == 0u &&
              no_release.State().data == 0u &&
              no_release.State().length == 0u &&
              no_release.State().capacity == 0u &&
              runtime.events.empty(),
          "zero flag clears fields without reading or freeing text");

    pal4::StringBuffer null_text({0u, 8u, 16u});
    Check(null_text.SetErrorFlag(1u, runtime) == 0u &&
              runtime.events.empty(),
          "nonzero flag with null text returns zero and clears fields");

    for (const std::uint8_t reference : {0u, 0xffu}) {
        runtime.events.clear();
        runtime.bytes[0x1200u] = reference;
        pal4::StringBuffer buffer({0x1201u, 8u, 16u});
        Check(buffer.SetErrorFlag(1u, runtime) == 0x7bu &&
                  runtime.events == std::vector<std::string>{
                      "read:4608", "free:4608"} &&
                  buffer.State().data == 0u,
              "zero and ff references free header and forward low free result");
    }

    runtime.events.clear();
    runtime.bytes[0x1200u] = 3u;
    pal4::StringBuffer shared({0x1201u, 8u, 16u});
    Check(shared.SetErrorFlag(1u, runtime) == 0u &&
              runtime.bytes[0x1200u] == 2u &&
              runtime.events == std::vector<std::string>{
                  "read:4608", "write:4608"} &&
              shared.State().data == 0u,
          "ordinary reference decrements and return is header low byte");
}

void CheckSetSize() {
    ResetRuntime runtime;
    pal4::StringBuffer buffer({0x1201u, 7u, 16u});
    Check(buffer.SetSize(3u, runtime) == 3u &&
              buffer.State().data == 0x1201u &&
              buffer.State().length == 3u &&
              buffer.State().capacity == 16u &&
              runtime.bytes[0x1204u] == 0u &&
              runtime.events == std::vector<std::string>{"write:4612"},
          "set-size stores length, terminates data, and returns new size");
}

void CheckAllocateTextLengthAndSharedStorage() {
    Runtime runtime;
    Check(pal4::StringBuffer::MaxSize() == 0xfffffffdu,
          "string max-size helper returns the observed constant");
    pal4::StringBuffer oversized({0u, 0u, 0u});
    bool length_error = false;
    try {
        (void)oversized.AllocateText(0xfffffffeu, 0u, runtime);
    } catch (const std::length_error&) {
        length_error = true;
    }
    Check(length_error && runtime.events == std::vector<std::string>{"length"},
          "allocator rejects lengths above fffffffd");

    runtime.events.clear();
    runtime.bytes[0x2000u] = 3u;
    pal4::StringBuffer shared({0x2001u, 4u, 8u});
    Check(shared.AllocateText(12u, 1u, runtime) &&
              shared.State().data == 0x5001u &&
              runtime.events == std::vector<std::string>{
                  "read:8192", "reallocate:12"},
          "nonzero request on shared storage calls reallocator directly");

    runtime.events.clear();
    pal4::StringBuffer shared_zero({0x2001u, 4u, 8u});
    Check(!shared_zero.AllocateText(0u, 0u, runtime) &&
              runtime.bytes[0x2000u] == 2u &&
              shared_zero.State().data == 0u &&
              shared_zero.State().length == 0u &&
              shared_zero.State().capacity == 0u &&
              runtime.events == std::vector<std::string>{
                  "read:8192", "write:8192"},
          "zero request on shared storage decrements and clears all fields");
}

void CheckAllocateTextZeroModes() {
    Runtime runtime;
    runtime.bytes[0x2000u] = 0u;
    pal4::StringBuffer keep({0x2001u, 5u, 16u});
    Check(!keep.AllocateText(0u, 0u, runtime) &&
              keep.State().data == 0x2001u &&
              keep.State().length == 0u &&
              keep.State().capacity == 16u &&
              runtime.bytes[0x2001u] == 0u &&
              runtime.events.back() == "write:8193",
          "mode zero keeps unique buffer and writes empty terminator");

    runtime.events.clear();
    runtime.bytes[0x2000u] = 0xffu;
    pal4::StringBuffer release({0x2001u, 5u, 16u});
    Check(!release.AllocateText(0u, 1u, runtime) &&
              release.State().data == 0u &&
              release.State().length == 0u &&
              release.State().capacity == 0u &&
              runtime.events == std::vector<std::string>{
                  "read:8192", "free:8192"},
          "mode one frees unique text and clears state");
}

void CheckAllocateTextCapacityPaths() {
    Runtime runtime;
    pal4::StringBuffer enough({0u, 0u, 12u});
    Check(enough.AllocateText(10u, 0u, runtime) &&
              enough.State().capacity == 12u && runtime.events.empty(),
          "mode zero reuses adequate capacity");

    runtime.events.clear();
    pal4::StringBuffer grow({0u, 0u, 4u});
    Check(grow.AllocateText(10u, 0u, runtime) &&
              grow.State().data == 0x5001u &&
              runtime.events == std::vector<std::string>{"reallocate:10"},
          "mode zero grows insufficient capacity");

    runtime.events.clear();
    runtime.bytes[0x2000u] = 0u;
    pal4::StringBuffer small({0x2001u, 3u, 20u});
    Check(small.AllocateText(10u, 1u, runtime) &&
              small.State().data == 0x2001u &&
              runtime.events == std::vector<std::string>{"read:8192"},
          "mode one reuses a small-capacity unique buffer");

    runtime.events.clear();
    pal4::StringBuffer large({0x2001u, 3u, 40u});
    Check(large.AllocateText(10u, 1u, runtime) &&
              large.State().data == 0x5001u &&
              large.State().capacity == 18u &&
              runtime.length_before_reallocate == 0u &&
              runtime.events == std::vector<std::string>{
                  "read:8192", "read:8192", "free:8192",
                  "reallocate:10"},
          "mode one frees large-capacity text before reallocation");
}

}  // namespace

int main() {
    CheckRangeAndLengthErrors();
    CheckSharedWholeSource();
    CheckSubstringCopyAndReallocation();
    CheckEmptySelectionBranches();
    CheckSelfSubstring();
    CheckUnshareableSourceUsesExistingCapacity();
    CheckSetErrorFlag();
    CheckSetSize();
    CheckAllocateTextLengthAndSharedStorage();
    CheckAllocateTextZeroModes();
    CheckAllocateTextCapacityPaths();
    std::cout << "11 string buffer assignment/reset test cases passed\n";
}
