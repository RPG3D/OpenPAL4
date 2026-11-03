#include "pal4/player_manager_global_data.h"

#include <cstdlib>
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

struct Runtime final : pal4::PlayerManagerGlobalRuntimeStub {
    std::vector<std::string> events;
    std::uint32_t destination{0x5000u};
    std::string_view expected_name{"LeaderPos"};
    std::uint32_t expected_callback{0x441040u};

    void SetErrorFlag(pal4::PlayerManagerGlobalText& text,
                      std::uint8_t flag) override {
        Check(flag == 1u && text.initial_byte == 0xa7u &&
                  text.data == 0u && text.length == 0u &&
                  text.capacity == 0u,
              "string reset follows byte write and zero initialization");
        events.emplace_back("reset");
    }
    void Reallocate(pal4::PlayerManagerGlobalText& text,
                    std::uint32_t length) override {
        Check(events == std::vector<std::string>{"reset"} &&
                  length == expected_name.size(),
              "reallocation uses source text length");
        events.emplace_back("allocate");
        text.data = destination;
        text.capacity = 0x20u;
    }
    void CopyText(std::uint32_t address,
                  std::string_view text) override {
        Check(events.back() == "allocate" && address == destination &&
                  text == expected_name, "copies exact source text");
        events.emplace_back("copy");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        Check(events.back() == "copy" &&
                  address == destination + expected_name.size() &&
                  value == 0u, "writes trailing NUL after text copy");
        events.emplace_back("nul");
    }
    int RegisterExit(std::uint32_t callback) override {
        Check(events.back() == "nul" && callback == expected_callback,
              "cleanup registration follows full text initialization");
        events.emplace_back("register");
        return -17;
    }
};

void TestInitializeName() {
    pal4::PlayerManagerGlobalText text{};
    text.data = 0x1111u;
    text.length = 23u;
    text.capacity = 24u;
    Runtime runtime;
    pal4::PlayerManagerGlobalData::InitializeName(text, 0xa7u, runtime);
    Check(text.initial_byte == 0xa7u && text.data == 0x5000u &&
              text.length == 9u && text.capacity == 0x20u &&
              runtime.events ==
                  std::vector<std::string>({"reset", "allocate", "copy", "nul"}),
          "name initializer updates only the observed string fields");
}

void TestInitializeAndRegister() {
    pal4::PlayerManagerGlobalText text{};
    Runtime runtime;
    Check(pal4::PlayerManagerGlobalData::Initialize(
              text, 0xa7u, runtime) == -17 &&
              runtime.events == std::vector<std::string>(
                  {"reset", "allocate", "copy", "nul", "register"}),
          "top-level initializer forwards CRT registration result");
}

struct CleanupRuntime final : pal4::PlayerManagerGlobalRuntimeStub {
    std::uint8_t header_value{};
    std::uint32_t header_address{0x4fffu};
    std::uint32_t reads{};
    std::uint32_t writes{};
    std::uint32_t frees{};

    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == header_address,
              "cleanup reads byte before data pointer");
        ++reads;
        return header_value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == header_address && value == header_value - 1u,
              "shared header reference count decrements");
        ++writes;
    }
    void Free(std::uint32_t address) override {
        Check(address == header_address,
              "unshared text frees header, not data pointer");
        ++frees;
    }
};

void TestCleanupName() {
    for (const auto header : {0u, 0xffu, 1u, 2u}) {
        pal4::PlayerManagerGlobalText text{};
        text.initial_byte = 0xabu;
        text.data = 0x5000u;
        text.length = 9u;
        text.capacity = 0x20u;
        std::uint8_t guard = 0x80u;
        CleanupRuntime runtime;
        runtime.header_value = static_cast<std::uint8_t>(header);
        pal4::PlayerManagerGlobalData::CleanupName(text, guard, runtime);
        Check(guard == 0x81u && text.initial_byte == 0xabu &&
                  text.data == 0u && text.length == 0u &&
                  text.capacity == 0u && runtime.reads == 1u &&
                  runtime.frees == (header == 0u || header == 0xffu ? 1u : 0u) &&
                  runtime.writes == (header == 0u || header == 0xffu ? 0u : 1u),
              "cleanup branches by reference header and clears text");
        pal4::PlayerManagerGlobalData::CleanupName(text, guard, runtime);
        Check(runtime.reads == 1u,
              "cleanup guard prevents duplicate header access");
    }
    pal4::PlayerManagerGlobalText empty{};
    empty.length = 7u;
    empty.capacity = 8u;
    std::uint8_t guard{};
    CleanupRuntime runtime;
    pal4::PlayerManagerGlobalData::CleanupName(empty, guard, runtime);
    Check(guard == 1u && empty.data == 0u && empty.length == 0u &&
              empty.capacity == 0u && runtime.reads == 0u,
          "null text still sets guard and clears metadata");
}

void TestRotationInitialization() {
    pal4::PlayerManagerGlobalText text{};
    Runtime runtime;
    runtime.expected_name = "LeaderRot";
    runtime.expected_callback = 0x4411a0u;
    Check(pal4::PlayerManagerGlobalData::InitializeRotationName(
              text, 0xa7u, runtime) == 0x5000u &&
              text.length == 9u &&
              runtime.events ==
                  std::vector<std::string>({"reset", "allocate", "copy", "nul"}),
          "recovered 0x4410b0 initializes rotation text and returns data");
    runtime.events.clear();
    Check(pal4::PlayerManagerGlobalData::InitializeRotation(
              text, 0xa7u, runtime) == -17 &&
              runtime.events == std::vector<std::string>(
                  {"reset", "allocate", "copy", "nul", "register"}),
          "rotation initializer registers cleanup after text copy");
}

void TestRotationCleanup() {
    pal4::PlayerManagerGlobalText position{};
    position.data = 0x5000u;
    position.length = 9u;
    pal4::PlayerManagerGlobalText rotation{};
    rotation.data = 0x6000u;
    rotation.length = 9u;
    std::uint8_t guard = 0x80u;
    CleanupRuntime runtime;
    runtime.header_value = 2u;
    runtime.header_address = 0x5fffu;
    pal4::PlayerManagerGlobalData::CleanupRotationName(
        rotation, guard, runtime);
    Check(guard == 0x82u && rotation.data == 0u &&
              rotation.length == 0u && runtime.reads == 1u &&
              runtime.writes == 1u && runtime.frees == 0u,
          "rotation cleanup decrements shared header and sets guard bit two");
    runtime.header_address = 0x4fffu;
    runtime.header_value = 0u;
    pal4::PlayerManagerGlobalData::CleanupName(position, guard, runtime);
    Check(guard == 0x83u && position.data == 0u &&
              runtime.reads == 2u && runtime.frees == 1u,
          "position cleanup uses independent guard bit one");
    pal4::PlayerManagerGlobalData::CleanupRotationName(
        rotation, guard, runtime);
    Check(runtime.reads == 2u,
          "rotation cleanup does not run twice after guard set");
}

}  // namespace

int main() {
    TestInitializeName();
    TestInitializeAndRegister();
    TestCleanupName();
    TestRotationInitialization();
    TestRotationCleanup();
}
