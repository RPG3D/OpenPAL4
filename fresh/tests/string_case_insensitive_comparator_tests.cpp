#include "pal4/string_case_insensitive_comparator.h"

#include <algorithm>
#include <cctype>
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

class Runtime final : public pal4::IStringBufferRuntime {
public:
    std::unordered_map<std::uint32_t, std::uint8_t> memory;
    std::vector<std::string> calls;
    std::uint32_t next_allocation = 0x2000u;
    bool fail_compare = false;

    void Put(std::uint32_t address, const std::string& value) {
        for (std::size_t i = 0; i < value.size(); ++i)
            memory[address + static_cast<std::uint32_t>(i)] =
                static_cast<std::uint8_t>(value[i]);
        memory[address + static_cast<std::uint32_t>(value.size())] = 0u;
    }
    std::string Text(std::uint32_t address) {
        std::string value;
        while (ReadByte(address) != 0u) value.push_back(ReadByte(address++));
        return value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate:" + std::to_string(bytes));
        const auto result = next_allocation;
        next_allocation += 0x100u;
        return result;
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
        return 0u;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return memory[address];
    }
    std::uint32_t ReadDword(std::uint32_t) override { return 0u; }
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t bytes) override {
        for (std::uint32_t i = 0; i < bytes; ++i)
            memory[destination + i] = memory[source + i];
    }
    void WriteDword(std::uint32_t, std::uint32_t) override {}
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        memory[address] = value;
    }
    std::uint32_t FindSubstring(std::uint32_t,
                                std::uint32_t) override { return 0u; }
    std::uint32_t FindCharacter(std::uint32_t,
                                std::int32_t) override { return 0u; }
    std::uint32_t FindLastCharacter(std::uint32_t,
                                    std::int32_t) override { return 0u; }
    std::int32_t CompareCaseInsensitive(std::uint32_t left,
                                        std::uint32_t right) override {
        calls.emplace_back("compare");
        if (fail_compare) throw std::runtime_error("compare failed");
        auto a = Text(left);
        auto b = Text(right);
        std::transform(a.begin(), a.end(), a.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        std::transform(b.begin(), b.end(), b.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return a.compare(b);
    }
    void ConvertUppercase(std::uint32_t) override {}
    void ConvertLowercase(std::uint32_t) override {}
};

void Cases() {
    Runtime runtime;
    runtime.Put(0x1000u, "Alpha");
    runtime.Put(0x1100u, "aLPHa");
    Check(pal4::StringCaseInsensitiveComparator::CompareManagerKey(
              0x1000u, 0x1100u, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{
                  "allocate:13", "allocate:13", "compare",
                  "free:8448", "free:8192"},
          "manager-key entry compares and cleans in reverse order");
    runtime.calls.clear();
    runtime.next_allocation = 0x2000u;
    Check(pal4::StringCaseInsensitiveComparator::Compare(
              0x1000u, 0x1100u, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{
                  "allocate:13", "allocate:13", "compare",
                  "free:8448", "free:8192"},
          "case-insensitive comparison allocates and frees in reverse order");

    runtime.calls.clear();
    runtime.Put(0x1200u, "beta");
    Check(pal4::StringCaseInsensitiveComparator::Compare(
              0x1000u, 0x1200u, runtime) < 0 &&
              runtime.calls[2] == "compare" &&
              runtime.calls[3] == "free:8960" &&
              runtime.calls[4] == "free:8704",
          "ordering result propagates and both buffers are released");

    runtime.calls.clear();
    runtime.fail_compare = true;
    bool caught = false;
    try {
        static_cast<void>(pal4::StringCaseInsensitiveComparator::Compare(
            0x1000u, 0x1100u, runtime));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls == std::vector<std::string>{
              "allocate:13", "allocate:13", "compare",
              "free:9472", "free:9216"},
          "comparison exception releases both temporary buffers");
}

}  // namespace

int main() {
    try {
        Cases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
