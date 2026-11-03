#include "pal4/string_buffer.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IStringAssignRuntime {
public:
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;

    void ThrowRange() override { throw std::out_of_range("range"); }
    void ThrowLength() override { throw std::length_error("length"); }
    void DetachAndReset(pal4::StringBufferState&) override {
        Check(false, "append does not detach through assignment helper");
    }
    bool AllocateText(pal4::StringBufferState&, std::uint32_t,
                      std::uint8_t) override {
        Check(false, "append does not allocate through assignment helper");
        return false;
    }
    void SetTextSize(pal4::StringBufferState&, std::uint32_t) override {
        Check(false, "append sets length directly");
    }
    void SetErrorFlag(pal4::StringBufferState&, std::uint8_t) override {
        Check(false, "nonempty append does not clear destination");
    }
    void Reallocate(pal4::StringBufferState& state,
                    std::uint32_t length) override {
        events.emplace_back("reallocate:" + std::to_string(length));
        const auto old = state.data;
        state.data = 0x8001u;
        state.capacity = length + 8u;
        for (std::uint32_t i = 0; i < state.length; ++i) {
            bytes[state.data + i] = bytes.at(old + i);
        }
        bytes[state.data - 1u] = 0u;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t length) override {
        events.emplace_back("copy:" + std::to_string(length));
        for (std::uint32_t i = 0; i < length; ++i) {
            bytes[destination + i] = bytes.at(source + i);
        }
    }
    void Free(std::uint32_t) override {
        Check(false, "append does not free directly");
    }
    void Put(std::uint32_t address, const std::string& value) {
        for (std::uint32_t i = 0; i < value.size(); ++i) {
            bytes[address + i] = static_cast<std::uint8_t>(value[i]);
        }
        bytes[address + static_cast<std::uint32_t>(value.size())] = 0u;
    }
    std::string Text(const pal4::StringBuffer& value) const {
        std::string text;
        for (std::uint32_t i = 0; i < value.State().length; ++i) {
            text.push_back(static_cast<char>(bytes.at(value.State().data + i)));
        }
        return text;
    }
};

void TestSelectionAndCapacity() {
    Runtime runtime;
    runtime.Put(0x1001u, "abcdef");
    runtime.Put(0x2001u, "XY");
    runtime.bytes[0x2000u] = 0u;
    pal4::StringBuffer source({0x1001u, 6u, 8u});
    pal4::StringBuffer target({0x2001u, 2u, 8u});
    auto& result = target.AppendRange(source, 2u, 2u, runtime);
    Check(&result == &target && runtime.Text(target) == "XYcd" &&
              target.State().length == 4u && runtime.bytes.at(0x2005u) == 0u &&
              runtime.events == std::vector<std::string>{"copy:2"},
          "bounded range appends in available capacity");
    (void)target.AppendRange(source, 4u, 100u, runtime);
    Check(runtime.Text(target) == "XYcdef", "count clamps to source suffix");
    const auto prior = runtime.events.size();
    (void)target.AppendRange(source, 6u, 10u, runtime);
    Check(runtime.events.size() == prior && runtime.Text(target) == "XYcdef",
          "empty selection leaves destination alone");
}

void TestErrorsAndReallocation() {
    Runtime runtime;
    runtime.Put(0x1001u, "abc");
    runtime.Put(0x2001u, "XY");
    runtime.bytes[0x2000u] = 2u;
    pal4::StringBuffer source({0x1001u, 3u, 8u});
    pal4::StringBuffer target({0x2001u, 2u, 8u});
    try {
        (void)target.AppendRange(source, 4u, 1u, runtime);
        Check(false, "range exception expected");
    } catch (const std::out_of_range&) {
        Check(target.State().data == 0x2001u, "range error preserves state");
    }
    (void)target.AppendRange(source, 0u, 3u, runtime);
    Check(runtime.events == std::vector<std::string>{"reallocate:5", "copy:3"} &&
              runtime.Text(target) == "XYabc" &&
              runtime.bytes.at(0x2000u) == 2u,
          "shared text reallocates before writing");

    pal4::StringBuffer too_long({0x2001u, 0xfffffffcu, 0xfffffffcu});
    try {
        (void)too_long.AppendRange(source, 0u, 2u, runtime);
        Check(false, "length exception expected");
    } catch (const std::length_error&) {
        Check(too_long.State().length == 0xfffffffcu,
              "length error preserves state");
    }
}

void TestNullSourceUsesEmptySentinel() {
    Runtime runtime;
    runtime.bytes[0x841964u] = 0u;
    runtime.Put(0x2001u, "A");
    runtime.bytes[0x2000u] = 0u;
    pal4::StringBuffer source({0u, 1u, 0u});
    pal4::StringBuffer target({0x2001u, 1u, 4u});
    (void)target.AppendRange(source, 0u, 1u, runtime);
    Check(target.State().length == 2u && runtime.bytes.at(0x2002u) == 0u,
          "null source pointer uses original empty sentinel");
}

void TestRawAppend() {
    Runtime runtime;
    runtime.Put(0x1001u, "abcd");
    runtime.Put(0x2001u, "XY");
    runtime.bytes[0x2000u] = 0u;
    pal4::StringBuffer target({0x2001u, 2u, 3u});
    auto& result = target.AppendRaw(0x1001u, 4u, runtime);
    Check(&result == &target && runtime.Text(target) == "XYabcd" &&
              runtime.events == std::vector<std::string>{"reallocate:6", "copy:4"},
          "raw bytes append after capacity growth");
    const auto prior = runtime.events.size();
    (void)target.AppendRaw(0x1001u, 0u, runtime);
    Check(runtime.events.size() == prior && runtime.Text(target) == "XYabcd",
          "zero-byte append is unchanged");

    pal4::StringBuffer too_long({0x2001u, 0xfffffffdu, 0xfffffffdu});
    try {
        (void)too_long.AppendRaw(0x1001u, 1u, runtime);
        Check(false, "raw append length exception expected");
    } catch (const std::length_error&) {
        Check(too_long.State().length == 0xfffffffdu,
              "raw append error preserves state");
    }
}

void TestCStringAppend() {
    Runtime runtime;
    runtime.Put(0x1001u, "abc");
    runtime.Put(0x2001u, "XY");
    runtime.bytes[0x2000u] = 0u;
    pal4::StringBuffer target({0x2001u, 2u, 4u});
    auto& returned = target.AppendCString(0x1001u, runtime);
    Check(&returned == &target && runtime.Text(target) == "XYabc" &&
              target.State().length == 5u &&
              runtime.events ==
                  std::vector<std::string>{"reallocate:5", "copy:3"},
          "C-string append measures source then appends through growth");
    runtime.Put(0x3001u, "");
    const auto events = runtime.events.size();
    (void)target.AppendCString(0x3001u, runtime);
    Check(runtime.events.size() == events && runtime.Text(target) == "XYabc",
          "empty C string leaves buffer unchanged");

    Runtime shared_runtime;
    shared_runtime.Put(0x1001u, "Z");
    shared_runtime.Put(0x2001u, "AB");
    shared_runtime.bytes[0x2000u] = 2u;
    pal4::StringBuffer shared({0x2001u, 2u, 10u});
    (void)shared.AppendCString(0x1001u, shared_runtime);
    Check(shared_runtime.Text(shared) == "ABZ" &&
              shared_runtime.events ==
                  std::vector<std::string>{"reallocate:3", "copy:1"},
          "shared destination detaches even when capacity is sufficient");
}

}  // namespace

int main() {
    TestSelectionAndCapacity();
    TestErrorsAndReallocation();
    TestNullSourceUsesEmptySentinel();
    TestRawAppend();
    TestCStringAppend();
    return EXIT_SUCCESS;
}
