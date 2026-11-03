#include "pal4/string_allocated_copy.h"

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

class Runtime final : public pal4::IStringBufferRuntime,
                      public pal4::IStringAllocatedCopyRuntime {
public:
    std::unordered_map<std::uint32_t, std::uint8_t> memory;
    std::vector<std::string> calls;
    std::uint32_t next_allocation = 0x2000u;
    bool fail_copy = false;

    void Put(std::uint32_t address, const std::string& text) {
        for (std::size_t i = 0; i < text.size(); ++i)
            memory[address + static_cast<std::uint32_t>(i)] =
                static_cast<std::uint8_t>(text[i]);
        memory[address + static_cast<std::uint32_t>(text.size())] = 0u;
    }
    std::string Text(std::uint32_t address) {
        std::string text;
        for (std::size_t i = 0; i < 64u; ++i) {
            const auto value = memory[address + static_cast<std::uint32_t>(i)];
            if (value == 0u) break;
            text.push_back(static_cast<char>(value));
        }
        return text;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate:" + std::to_string(bytes));
        const auto result = next_allocation;
        next_allocation += 0x100u;
        return result;
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
        return 0x88u;
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
    std::int32_t CompareCaseInsensitive(std::uint32_t,
                                        std::uint32_t) override { return 0; }
    void ConvertUppercase(std::uint32_t) override {}
    void ConvertLowercase(std::uint32_t) override {}
    void CopyCString(std::uint32_t destination,
                     std::uint32_t source) override {
        calls.emplace_back("copy cstring");
        if (fail_copy) throw std::runtime_error("copy failed");
        const auto text = Text(source);
        Put(destination, text);
    }
};

void Cases() {
    Runtime runtime;
    pal4::StringAllocatedCopy copy(runtime, runtime);
    runtime.Put(0x1000u, "Camera");
    std::uint32_t destination = 0xdeadbeefu;
    Check(copy.CopyFromCString(destination, 0x1000u) == 0x88u &&
              destination == 0x2100u && runtime.Text(destination) == "Camera" &&
              runtime.calls == std::vector<std::string>{
                  "allocate:14", "allocate:7", "copy cstring", "free:8192"},
          "copy allocates temporary and destination then returns cleanup result");

    runtime.calls.clear();
    runtime.Put(0x1100u, "");
    Check(copy.CopyFromCString(destination, 0x1100u) == 0x88u &&
              destination == 0x2300u && runtime.ReadByte(destination) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "allocate:8", "allocate:1", "copy cstring", "free:8704"},
          "empty source still copies its terminator into one-byte destination");

    runtime.calls.clear();
    runtime.fail_copy = true;
    bool caught = false;
    try {
        static_cast<void>(copy.CopyFromCString(destination, 0x1000u));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && destination == 0x2500u &&
              runtime.calls == std::vector<std::string>{
                  "allocate:14", "allocate:7", "copy cstring", "free:9216"},
          "copy failure releases temporary while retaining assigned destination");
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
