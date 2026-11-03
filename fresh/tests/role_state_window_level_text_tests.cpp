#include "pal4/role_state_window_level_text.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IRoleStateWindowLevelRuntime {
    std::map<std::uint32_t, std::uint8_t> memory{{0x1000u, '0'},
                                                 {0x1001u, 0u}};
    std::vector<std::string> calls;
    std::uint32_t next = 0x5000u;
    std::uint32_t allocation_count = 0u;
    std::uint32_t throw_on_allocation = 0u;

    std::uint32_t ZeroLiteralAddress() override { return 0x1000u; }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        ++allocation_count;
        calls.push_back("allocate:" + std::to_string(bytes));
        if (allocation_count == throw_on_allocation)
            throw std::runtime_error("allocate");
        const auto result = next;
        next += bytes + 16u;
        return result;
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        return 0u;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return memory[address];
    }
    std::uint32_t ReadDword(std::uint32_t) override { return 0u; }
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t bytes) override {
        calls.push_back("copy:" + std::to_string(bytes));
        for (std::uint32_t i = 0u; i < bytes; ++i)
            memory[destination + i] = memory[source + i];
    }
    void WriteDword(std::uint32_t, std::uint32_t) override {}
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        memory[address] = value;
    }
    std::uint32_t FindSubstring(std::uint32_t, std::uint32_t) override {
        return 0u;
    }
    std::uint32_t FindCharacter(std::uint32_t, std::int32_t) override {
        return 0u;
    }
    std::uint32_t FindLastCharacter(std::uint32_t,
                                    std::int32_t) override { return 0u; }
    std::int32_t CompareCaseInsensitive(std::uint32_t,
                                        std::uint32_t) override { return 0; }
    void ConvertUppercase(std::uint32_t) override {}
    void ConvertLowercase(std::uint32_t) override {}

    std::string Text(std::uint32_t address) const {
        std::string result;
        for (std::uint32_t i = 0u; i < 64u; ++i) {
            auto found = memory.find(address + i);
            if (found == memory.end() || found->second == 0u) break;
            result.push_back(static_cast<char>(found->second));
        }
        return result;
    }
};

void CheckValue(std::int32_t input, const std::string& expected) {
    Runtime runtime;
    pal4::StringBuffer output;
    Check(&pal4::RoleStateWindowLevelText::FormatInto(
              output, input, runtime) == &output &&
              pal4::RoleStateWindowLevelText::FormatText(input) == expected &&
              runtime.Text(output.State().data) == expected &&
              output.State().length == expected.size() &&
              output.State().capacity == expected.size() + 8u,
          "level text bytes, metadata and return identity");
    if (input > 0 && input < 100) {
        Check(runtime.calls.size() == 6u &&
                  runtime.calls[0] == "allocate:32" &&
                  runtime.calls[1] == "allocate:1032" &&
                  runtime.calls[2] == "free:20480" &&
                  runtime.calls[3] ==
                      "allocate:" + std::to_string(expected.size() + 8u) &&
                  runtime.calls[4] ==
                      "copy:" + std::to_string(expected.size()) &&
                  runtime.calls[5].find("free:") == 0u,
              "temporary allocation, deep copy and cleanup order");
    } else {
        Check(runtime.calls == std::vector<std::string>{
                  "allocate:9", "copy:1"},
              "out-of-range values directly copy literal zero");
    }
}

void TestFormats() {
    CheckValue(-1, "0");
    CheckValue(0, "0");
    CheckValue(100, "0");
    CheckValue(999, "0");
    CheckValue(1, "1\n \n");
    CheckValue(9, "9\n \n");
    CheckValue(10, "1\n\n0\n \n ");
    CheckValue(42, "4\n0\n2\n \n ");
    CheckValue(99, "9\n0\n9\n \n ");
}

void TestUnwind() {
    Runtime runtime;
    runtime.throw_on_allocation = 2u;
    pal4::StringBuffer output;
    try {
        static_cast<void>(pal4::RoleStateWindowLevelText::FormatInto(
            output, 42, runtime));
        Check(false, "allocation should throw");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "allocate:32", "allocate:1032", "free:20480"},
              "temporary string freed on formatting failure");
    }
}

}  // namespace

int main() {
    TestFormats();
    TestUnwind();
}
