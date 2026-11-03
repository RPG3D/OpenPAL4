#include "pal4/effect_system_task_factory.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectSystemTaskFactoryRuntime {
    std::uint32_t allocation_result = 0x5000u;
    std::uint32_t size = 0u;
    std::uint32_t constructed = 0u;
    pal4::EffectSystemTaskKind kind{};
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        size = bytes;
        calls.emplace_back("allocate");
        return allocation_result;
    }
    std::uint32_t ConstructTask(pal4::EffectSystemTaskKind task_kind,
                                std::uint32_t object) override {
        kind = task_kind;
        constructed = object;
        calls.emplace_back("construct_task");
        return 0x6000u + static_cast<std::uint32_t>(task_kind);
    }
    void ConstructCommonTask(std::uint32_t object) override {
        Check(object == allocation_result,
              "inline factory constructs allocated object");
        calls.emplace_back("construct_common");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        writes.emplace_back(address, value);
        calls.emplace_back("word");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        writes.emplace_back(address, value);
        calls.emplace_back("byte");
    }
};

void TestExternalConstructors() {
    constexpr std::array<std::uint32_t, 24u> sizes{
        0x4cu, 0x4cu, 0x9cu, 0xe4u, 0xb8u, 0x88u,
        0xb8u, 0x50u, 0x4cu, 0x60u, 0x5cu, 0x50u,
        0x4cu, 0x4cu, 0x5cu, 0x90u, 0x4cu, 0x4cu,
        0xb8u, 0x54u, 0x90u, 0x54u, 0x5cu, 0x50u};
    Check(pal4::EffectSystemTaskFactory::kSizes == sizes,
          "all 24 IDA allocation sizes match the creator mapping");
    for (std::uint32_t index = 0u; index < 22u; ++index) {
        Runtime runtime;
        const auto kind = static_cast<pal4::EffectSystemTaskKind>(index);
        Check(pal4::EffectSystemTaskFactory::Create(kind, runtime) ==
                  0x6000u + index &&
                  runtime.size == sizes[index] &&
                  runtime.kind == kind &&
                  runtime.constructed == 0x5000u &&
                  runtime.calls == std::vector<std::string>{
                      "allocate", "construct_task"} &&
                  runtime.writes.empty(),
              "external factory allocates exact size and dispatches constructor");
    }
}

void TestAllocationFailure() {
    for (std::uint32_t index = 0u; index < 24u; ++index) {
        Runtime runtime;
        runtime.allocation_result = 0u;
        Check(pal4::EffectSystemTaskFactory::Create(
                  static_cast<pal4::EffectSystemTaskKind>(index),
                  runtime) == 0u &&
                  runtime.calls == std::vector<std::string>{"allocate"} &&
                  runtime.writes.empty(),
              "failed allocation never calls a constructor");
    }
}

void TestMessageInlineConstructor() {
    Runtime runtime;
    Check(pal4::EffectSystemTaskFactory::Create(
              pal4::EffectSystemTaskKind::Message, runtime,
              0xa5u) == 0x5000u &&
              runtime.size == 0x5cu &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "construct_common", "word", "byte",
                  "word", "word", "word", "word"} &&
              runtime.writes ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x504cu, 0u}, {0x5048u, 0xa5u},
                      {0x5050u, 0u}, {0x5054u, 0u},
                      {0x5058u, 0u}, {0x5000u, 0x84b2d8u}},
          "MessageTask writes its inline tail and derived vtable in IDA order");
}

void TestSlowMotionInlineConstructor() {
    Runtime runtime;
    Check(pal4::EffectSystemTaskFactory::Create(
              pal4::EffectSystemTaskKind::SlowMotion, runtime) == 0x5000u &&
              runtime.size == 0x50u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "construct_common", "word", "word"} &&
              runtime.writes ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x504cu, 0u}, {0x5000u, 0x84b318u}},
          "SlowMotionTask clears +76 then writes derived vtable");
}

}  // namespace

int main() {
    TestExternalConstructors();
    TestAllocationFailure();
    TestMessageInlineConstructor();
    TestSlowMotionInlineConstructor();
}
