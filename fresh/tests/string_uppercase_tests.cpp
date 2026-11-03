#include "pal4/string_uppercase.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IStringUppercaseRuntime {
    std::map<std::uint32_t, std::uint8_t> memory;
    std::vector<std::string> calls;
    std::uint32_t freed_result{0xabcdu};

    void Put(std::uint32_t address, const std::string& text) {
        for (const auto ch : text) memory[address++] = static_cast<std::uint8_t>(ch);
        memory[address] = 0u;
    }
    [[nodiscard]] std::string Get(std::uint32_t address) {
        std::string result;
        while (memory[address] != 0u) result.push_back(static_cast<char>(memory[address++]));
        return result;
    }
    [[nodiscard]] std::uint32_t EmptySource() override {
        calls.push_back("empty");
        return 0x1000u;
    }
    [[nodiscard]] std::uint32_t Duplicate(const std::uint32_t source) override {
        calls.push_back("duplicate");
        Put(0x3000u, Get(source));
        return 0x3000u;
    }
    [[nodiscard]] std::uint32_t ToUpper(const std::uint32_t text) override {
        calls.push_back("upper");
        for (auto address = text; memory[address] != 0u; ++address) {
            if (memory[address] >= 'a' && memory[address] <= 'z')
                memory[address] = static_cast<std::uint8_t>(memory[address] - 'a' + 'A');
        }
        return text;
    }
    [[nodiscard]] std::uint32_t Length(const std::uint32_t text) override {
        calls.push_back("length");
        return static_cast<std::uint32_t>(Get(text).size());
    }
    [[nodiscard]] std::uint8_t ReadByte(const std::uint32_t address) override {
        calls.push_back("read-reference");
        return memory[address];
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        calls.push_back("write-byte");
        memory[address] = value;
    }
    void SetErrorFlag(pal4::UppercaseStringState& state,
                      const std::uint8_t value) override {
        calls.push_back("flag");
        Check(value == 1u && state.prefix == 0x7777u, "error flag arguments");
    }
    void Reallocate(pal4::UppercaseStringState& state,
                    const std::uint32_t length) override {
        calls.push_back("reallocate");
        Check(length == Get(0x3000u).size(), "reallocate length");
        state.data = 0x4000u;
        state.capacity = length;
    }
    void Copy(const std::uint32_t destination,
              const std::uint32_t source,
              const std::uint32_t length) override {
        calls.push_back("copy");
        Check(source == 0x3000u, "copy source");
        for (std::uint32_t i = 0u; i < length; ++i)
            memory[destination + i] = memory[source + i];
    }
    [[nodiscard]] std::uint32_t Free(const std::uint32_t address) override {
        calls.push_back("free");
        Check(address == 0x3000u, "temporary freed");
        return freed_result;
    }
};

void TestSharedStringReallocation() {
    Runtime runtime;
    runtime.Put(0x2000u, "aBc");
    runtime.memory[0x1fffu] = 2u;
    pal4::UppercaseStringState state{0x7777u, 0x2000u, 3u, 20u};
    Check(pal4::StringUppercase::Convert(state, runtime) == 0xabcdu,
          "shared string return");
    Check(state.data == 0x4000u && state.length == 3u &&
              state.capacity == 3u && runtime.Get(state.data) == "ABC",
          "shared string reallocation and uppercase");
    Check(runtime.memory[0x1fffu] == 2u, "shared reference remains intact");
    Check(runtime.calls == std::vector<std::string>{
              "duplicate", "upper", "length", "read-reference", "reallocate",
              "copy", "write-byte", "free"}, "shared string call order");
}

void TestSharedEmptyString() {
    Runtime runtime;
    runtime.Put(0x2000u, "");
    runtime.memory[0x1fffu] = 3u;
    pal4::UppercaseStringState state{0x7777u, 0x2000u, 0u, 7u};
    (void)pal4::StringUppercase::Convert(state, runtime);
    Check(state.data == 0u && state.length == 0u && state.capacity == 0u &&
              runtime.memory[0x1fffu] == 2u, "shared empty releases reference");
    Check(runtime.calls == std::vector<std::string>{
              "duplicate", "upper", "length", "read-reference", "write-byte",
              "free"}, "shared empty avoids flag and reallocation");
}

void TestUniqueWithinCapacity() {
    Runtime runtime;
    runtime.Put(0x2000u, "q9");
    pal4::UppercaseStringState state{0x7777u, 0x2000u, 2u, 31u};
    (void)pal4::StringUppercase::Convert(state, runtime);
    Check(runtime.Get(0x2000u) == "Q9" && state.length == 2u &&
              state.capacity == 31u, "unique buffer reused");
    Check(runtime.calls == std::vector<std::string>{
              "duplicate", "upper", "length", "read-reference", "copy",
              "write-byte", "free"}, "unique buffer avoids flag and allocation");
}

void TestCapacityTriggers() {
    for (const auto capacity : {1u, 32u}) {
        Runtime runtime;
        runtime.Put(0x2000u, "ab");
        pal4::UppercaseStringState state{0x7777u, 0x2000u, 2u, capacity};
        (void)pal4::StringUppercase::Convert(state, runtime);
        Check(runtime.Get(state.data) == "AB" && state.length == 2u,
              "capacity trigger result");
        Check(runtime.calls == std::vector<std::string>{
                  "duplicate", "upper", "length", "read-reference", "flag",
                  "reallocate", "copy", "write-byte", "free"},
              "capacity trigger order");
    }
}

void TestEmptyAndSentinelReferences() {
    for (const auto references : {0u, 255u}) {
        Runtime runtime;
        runtime.Put(0x2000u, "");
        runtime.memory[0x1fffu] = static_cast<std::uint8_t>(references);
        pal4::UppercaseStringState state{0x7777u, 0x2000u, 0u, 8u};
        (void)pal4::StringUppercase::Convert(state, runtime);
        Check(state.data == 0x2000u && runtime.memory[0x1fffu] == references,
              "zero and sentinel references retained");
        Check(runtime.calls == std::vector<std::string>{
                  "duplicate", "upper", "length", "read-reference", "flag", "free"},
              "empty unique call order");
    }
}

void TestNullSource() {
    Runtime runtime;
    runtime.Put(0x1000u, "");
    pal4::UppercaseStringState state{0x7777u, 0u, 0u, 0u};
    (void)pal4::StringUppercase::Convert(state, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "empty", "duplicate", "upper", "length", "flag", "free"},
          "null source uses global empty string");
}

}  // namespace

int main() {
    TestSharedStringReallocation();
    TestSharedEmptyString();
    TestUniqueWithinCapacity();
    TestCapacityTriggers();
    TestEmptyAndSentinelReferences();
    TestNullSource();
}
