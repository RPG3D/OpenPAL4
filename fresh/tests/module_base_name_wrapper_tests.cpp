#include "pal4/module_base_name_wrapper.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IModuleBaseNameRuntime {
public:
    std::uint32_t CheckOperatingSystemPlatform() override {
        events.emplace_back("platform");
        return platform;
    }
    std::uint32_t LoadDebugFunctions() override {
        events.emplace_back("load-debug");
        return load_result;
    }
    std::uint32_t GetModuleBaseNameA(
        std::uint32_t process, std::uint32_t module,
        std::uint32_t output, std::uint32_t capacity) override {
        Check(process == 0x11u && module == 0x22u &&
                  output == 0x3000u && capacity == expected_capacity,
              "NT export receives all original arguments");
        events.emplace_back("base-name");
        return base_name_result;
    }
    std::uint32_t IsBadWritePointer(std::uint32_t address,
                                    std::uint32_t bytes) override {
        Check(address == 0x3000u && bytes == expected_capacity,
              "old-platform validation uses output and capacity");
        events.emplace_back("validate");
        return validation_result;
    }
    std::uint32_t GetModuleFileNameA(std::uint32_t module, char* output,
                                     std::uint32_t capacity) override {
        Check(module == 0x22u && capacity == 0x104u && output[0] == 0 &&
                  output[260] == 0,
              "fallback requests 260 characters from a zeroed local buffer");
        events.emplace_back("file-name");
        if (file_name_result == 0u) return 0u;
        std::memcpy(output, path.c_str(), path.size() + 1u);
        return static_cast<std::uint32_t>(path.size());
    }
    void CopyN(std::uint32_t output, const char* source,
               std::uint32_t capacity) override {
        Check(output == 0x3000u, "copy writes to caller output");
        events.emplace_back("copy");
        copied_source = source;
        copied_count = capacity;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(value == 0u, "extra byte is zero");
        events.emplace_back("extra-zero");
        extra_address = address;
    }
    void SetLastError(std::uint32_t error) override {
        Check(error == 0x57u, "bad output sets error 0x57");
        events.emplace_back("error");
    }
    void SetLastErrorEx(std::uint32_t error, std::uint32_t type) override {
        Check(error == 0x45au && type == 1u,
              "missing PSAPI sets extended error 0x45a");
        events.emplace_back("extended-error");
    }

    std::vector<std::string> events;
    std::string path = "C:\\PAL4\\module.dll";
    std::string copied_source;
    std::uint32_t platform = 0u;
    std::uint32_t load_result = 1u;
    std::uint32_t base_name_result = 19u;
    std::uint32_t validation_result = 0u;
    std::uint32_t file_name_result = 1u;
    std::uint32_t expected_capacity = 32u;
    std::uint32_t copied_count = 0xffffffffu;
    std::uint32_t extra_address = 0u;
};

std::uint32_t Run(Runtime& runtime, std::uint32_t capacity = 32u) {
    runtime.expected_capacity = capacity;
    return pal4::ModuleBaseNameWrapper::Get(
        0x11u, 0x22u, 0x3000u, capacity, runtime);
}

void CheckNtPath() {
    Runtime runtime;
    runtime.platform = 1u;
    Check(Run(runtime) == 19u &&
              runtime.events == std::vector<std::string>{
                  "platform", "load-debug", "base-name"},
          "NT branch delegates to GetModuleBaseNameA export");
    Runtime missing;
    missing.platform = 1u;
    missing.load_result = 0u;
    Check(Run(missing) == 0u &&
              missing.events == std::vector<std::string>{
                  "platform", "load-debug", "extended-error"},
          "missing export sets extended error and returns zero");
}

void CheckOldPlatformNames() {
    Runtime path;
    Check(Run(path) == 10u && path.copied_source == "module.dll" &&
              path.copied_count == 11u && path.extra_address == 0x300bu &&
              path.events == std::vector<std::string>{
                  "platform", "validate", "file-name", "copy", "extra-zero"},
          "fallback strips final backslash and writes an extra terminator");

    Runtime plain;
    plain.path = "PAL4.exe";
    plain.platform = 2u;
    Check(Run(plain) == 8u && plain.copied_source == "PAL4.exe" &&
              plain.copied_count == 9u && plain.extra_address == 0x3009u,
          "non-one platform result uses full filename without separator");

    Runtime short_buffer;
    Check(Run(short_buffer, 4u) == 3u &&
              short_buffer.copied_count == 4u &&
              short_buffer.extra_address == 0x3004u,
          "short capacity returns capacity minus one and writes past CopyN area");
    Runtime zero;
    Check(Run(zero, 0u) == 0xffffffffu &&
              zero.copied_count == 0u && zero.extra_address == 0x3000u,
          "zero capacity retains wrapped return and zero-byte write");
}

void CheckFallbackFailuresAndSignedCapacity() {
    Runtime bad;
    bad.validation_result = 1u;
    Check(Run(bad) == 0u &&
              bad.events == std::vector<std::string>{
                  "platform", "validate", "error"},
          "bad output pointer stops before filename query");
    Runtime non_one;
    non_one.validation_result = 2u;
    non_one.file_name_result = 0u;
    Check(Run(non_one) == 0u &&
              non_one.events == std::vector<std::string>{
                  "platform", "validate", "file-name"},
          "non-one validation proceeds and filename failure forwards zero");

    Runtime signed_size;
    Check(Run(signed_size, 0x80000000u) == 0x7fffffffu &&
              signed_size.copied_count == 0x80000000u &&
              signed_size.extra_address == 0x80003000u,
          "capacity comparison is signed 32-bit while address addition wraps");
}

}  // namespace

int main() {
    CheckNtPath();
    CheckOldPlatformNames();
    CheckFallbackFailuresAndSignedCapacity();
    std::cout << "3 module base-name wrapper test cases passed\n";
}
