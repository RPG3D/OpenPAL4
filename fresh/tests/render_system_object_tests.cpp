#include "pal4/render_system_object.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void WriteDword(std::array<std::uint8_t,
                           pal4::RenderSystemObjectState::kBytes>& bytes,
                const std::uint32_t offset, const std::uint32_t value) {
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        bytes[offset + index] =
            static_cast<std::uint8_t>(value >> (8u * index));
    }
}

class Runtime final : public pal4::IRenderSystemObjectRuntime {
public:
    Runtime() {
        object.Bytes().fill(0xa5u);
        external_bytes[0x7000u] = 'X';
    }
    std::uint32_t Allocate(const std::uint32_t size) override {
        events.emplace_back("allocate");
        Check(size == 0x124u, "render system allocation size");
        return allocation_result;
    }
    pal4::RenderSystemObjectState& Access(
        const std::uint32_t address) override {
        Check(address == 0x5000u, "object address");
        return object;
    }
    void ClearObject(const std::uint32_t address) override {
        events.emplace_back("clear-object");
        Check(address == 0x5000u, "base object address");
    }
    void InitializeString(const std::uint32_t address) override {
        events.emplace_back("string-init");
        Check(address == 0x5118u, "embedded string address");
        object.WriteDword(0x118u, 0x7000u);
        object.WriteDword(0x11cu, 3u);
        object.WriteDword(0x120u, 4u);
    }
    void InitializeSecondaryBase(const std::uint32_t address) override {
        events.emplace_back("secondary-base");
        Check(address == 0x5000u, "secondary base receives object");
    }
    void ClearCollection(const std::uint32_t address) override {
        events.emplace_back("clear-collection");
        Check(address == 0x5000u, "collection helper receives object");
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        events.emplace_back("write-byte");
        Check(address == 0x7000u && value == 0u,
              "destructor zeros first byte of embedded string data");
        external_bytes[address] = value;
    }
    void CleanupString(const std::uint32_t address) override {
        events.emplace_back("string-cleanup");
        Check(address == 0x5118u && object.ReadDword(0x11cu) == 0u &&
                  external_bytes[0x7000u] == 0u,
              "string cleanup follows byte and size reset");
    }
    void CleanupBase(const std::uint32_t address) override {
        events.emplace_back("base-cleanup");
        Check(address == 0x5000u, "base cleanup receives object");
    }
    void Free(const std::uint32_t address) override {
        events.emplace_back("free");
        Check(address == 0x5000u, "free receives constructed address");
    }

    std::uint32_t allocation_result = 0x5000u;
    pal4::RenderSystemObjectState object;
    std::unordered_map<std::uint32_t, std::uint8_t> external_bytes;
    std::vector<std::string> events;
};

void AllocationAndConstructor() {
    Runtime runtime;
    Check(pal4::RenderSystemObject::Create(runtime) == 0x5000u &&
              runtime.events ==
                  std::vector<std::string>{
                      "allocate", "clear-object", "string-init",
                      "secondary-base", "clear-collection"},
          "factory allocation and constructor helper order");
    auto expected =
        std::array<std::uint8_t,
                   pal4::RenderSystemObjectState::kBytes>{};
    expected.fill(0xa5u);
    WriteDword(expected, 0u, 0x40a040u);
    WriteDword(expected, 4u, 0x40a0b0u);
    WriteDword(expected, 8u, 0x40a010u);
    for (std::uint32_t offset = 0x2cu; offset <= 0x64u; offset += 4u) {
        WriteDword(expected, offset, 0u);
    }
    WriteDword(expected, 0x70u, 0u);
    WriteDword(expected, 0x74u, 0u);
    WriteDword(expected, 0x118u, 0x7000u);
    WriteDword(expected, 0x11cu, 3u);
    WriteDword(expected, 0x120u, 4u);
    Check(runtime.object.Bytes() == expected,
          "constructor writes callback table, fields and string while "
          "preserving observed gaps");
}

void DestructorAndFree() {
    Runtime runtime;
    (void)pal4::RenderSystemObject::Construct(0x5000u, runtime);
    runtime.events.clear();
    pal4::RenderSystemObject::Destroy(0x5000u, runtime);
    Check(runtime.events ==
              std::vector<std::string>{
                  "clear-collection", "write-byte", "string-cleanup",
                  "base-cleanup", "free"} &&
              runtime.object.ReadDword(0x2cu) == 0u &&
              runtime.object.ReadDword(0x11cu) == 0u &&
              runtime.external_bytes[0x7000u] == 0u,
          "destructor resets callback/string then factory frees");
}

void NullFactoryAndDestroy() {
    Runtime runtime;
    runtime.allocation_result = 0u;
    Check(pal4::RenderSystemObject::Create(runtime) == 0u &&
              runtime.events == std::vector<std::string>{"allocate"},
          "null allocation skips constructor");
    runtime.events.clear();
    pal4::RenderSystemObject::Destroy(0u, runtime);
    Check(runtime.events.empty(), "null destroy does nothing");
}

}  // namespace

int main() {
    AllocationAndConstructor();
    DestructorAndFree();
    NullFactoryAndDestroy();
    std::cout << "3 render system object lifecycle paths passed\n";
}
