#include "pal4/script_skill_global_record.h"
#include "pal4/crt_noop_atexit_registration.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct GlobalRuntime final : pal4::IScriptSkillGlobalRuntime {
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return 0u;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read-byte " + std::to_string(address));
        return 0u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("word " + std::to_string(address) + " " +
                        std::to_string(value));
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("byte " + std::to_string(address) + " " +
                        std::to_string(value));
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
        return 0u;
    }
    void ResetNested(std::uint32_t address) override {
        calls.push_back("reset " + std::to_string(address));
    }
    int RegisterAtExit(std::uint32_t callback) override {
        calls.push_back("atexit " + std::to_string(callback));
        return 0;
    }
};

struct CounterRuntime final : pal4::IScriptSkillContextRuntime {
    std::uint32_t limit = 0u;
    std::vector<std::uint32_t> reads;
    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return limit;
    }
    std::uint32_t InvokeVirtual(std::uint32_t, std::uint32_t,
                                std::span<const std::uint32_t>) override {
        throw std::runtime_error("counter should not invoke a virtual method");
    }
    std::uint32_t SkillSystemInstance() override {
        throw std::runtime_error("counter should not query the skill system");
    }
    void ProcessSkillChain(std::uint32_t, std::uint32_t,
                           std::uint32_t, std::uint32_t,
                           std::uint32_t) override {
        throw std::runtime_error("counter should not process a skill chain");
    }
};

struct NoopRuntime final : pal4::ICrtNoopAtExitRuntime {
    std::vector<std::uint32_t> callbacks;
    int RegisterAtExit(std::uint32_t callback) override {
        callbacks.push_back(callback);
        return 9;
    }
};

void StaticFields() {
    GlobalRuntime runtime;
    const auto record = 0x12563478u;
    Check(pal4::ScriptSkillGlobalRecord::InitializeTextFields(
              record, runtime) == record && runtime.calls.size() == 16u,
          "text-field initializer returns record and writes sixteen fields");
    for (std::uint32_t i = 0u; i < 4u; ++i) {
        const auto base = record + 68u + i * 16u;
        const auto at = i * 4u;
        Check(runtime.calls[at] ==
                  "byte " + std::to_string(base) + " 18" &&
                  runtime.calls[at + 1u] ==
                      "word " + std::to_string(base + 4u) + " 0" &&
                  runtime.calls[at + 2u] ==
                      "word " + std::to_string(base + 8u) + " 0" &&
                  runtime.calls[at + 3u] ==
                      "word " + std::to_string(base + 12u) + " 0",
              "each text field starts with the address high byte");
    }
}

void LazyGlobal() {
    GlobalRuntime runtime;
    auto guard = std::uint8_t{0xa0u};
    Check(pal4::ScriptSkillGlobalRecord::GetOrInitialize(
              0x6000u, guard, runtime) == 0x6000u &&
              guard == 0xa0u && runtime.calls.empty(),
          "existing slot returns immediately");

    constexpr auto base = pal4::ScriptSkillGlobalRecord::kRecordAddress;
    Check(pal4::ScriptSkillGlobalRecord::GetOrInitialize(
              0u, guard, runtime) == base && guard == 0xa1u &&
              runtime.calls.size() == 25u &&
              runtime.calls[16] == "atexit 5804016",
          "first null slot initializes text then registers cleanup");
    constexpr std::array<std::uint32_t, 8> offsets{
        4u, 0u, 60u, 64u, 8u, 12u, 16u, 56u};
    constexpr std::array<std::uint32_t, 8> values{
        4u, 0xffffffffu, 0xffffffffu, 0xffffffffu,
        15u, 0u, 0u, 0u};
    for (std::size_t i = 0u; i < offsets.size(); ++i)
        Check(runtime.calls[17u + i] ==
                  "word " + std::to_string(base + offsets[i]) + " " +
                      std::to_string(values[i]),
              "global scalar fields follow IDA write order");

    runtime.calls.clear();
    Check(pal4::ScriptSkillGlobalRecord::GetOrInitialize(
              0u, guard, runtime) == base &&
              runtime.calls.size() == 8u &&
              runtime.calls.front() == "word " +
                  std::to_string(base + 4u) + " 4",
          "existing guard still resets scalar fields");
}

void CleanupAndCounter() {
    GlobalRuntime global;
    Check(pal4::ScriptSkillGlobalRecord::Release(global) == 0u &&
              global.calls.size() == 16u,
          "cleanup delegates all four text fields to existing release code");
    for (std::uint32_t i = 0u; i < 4u; ++i) {
        const auto field = pal4::ScriptSkillGlobalRecord::kRecordAddress +
                           (120u - i * 16u);
        Check(global.calls[i * 4u] ==
                  "read " + std::to_string(field) &&
                  global.calls[i * 4u + 1u] ==
                      "word " + std::to_string(field) + " 0",
              "cleanup visits skill text fields in reverse order");
    }

    pal4::ScriptSkillCounterState counter{};
    CounterRuntime runtime;
    Check(&pal4::ScriptSkillCounter::Construct(
              counter, 0x5000u, 0x7000u) == &counter &&
              counter.context.skill == 0x5000u &&
              counter.context.context == 0x7000u &&
              counter.count == 0u,
          "counter constructor calls context constructor and clears count");
    Check(pal4::ScriptSkillCounter::Add(counter, 7u) == 7u &&
              counter.count == 7u,
          "counter add returns argument and stores the sum");
    runtime.limit = 8u;
    Check(!pal4::ScriptSkillCounter::ReachedLimit(counter, runtime) &&
              runtime.reads == std::vector<std::uint32_t>{0x500cu},
          "counter reads signed limit at skill word +12");
    runtime.limit = 7u;
    Check(pal4::ScriptSkillCounter::ReachedLimit(counter, runtime),
          "counter reaches an equal limit");
    counter.count = 0xffffffffu;
    runtime.limit = 1u;
    Check(!pal4::ScriptSkillCounter::ReachedLimit(counter, runtime),
          "counter comparison is signed");
    counter.count = 0xfffffffeu;
    Check(pal4::ScriptSkillCounter::Add(counter, 3u) == 3u &&
              counter.count == 1u,
          "counter addition wraps as a 32-bit word");

    NoopRuntime noop;
    std::uint8_t guard = 0xf0u;
    Check(pal4::CrtNoopAtExitRegistration::Run(guard, noop) == 9 &&
              guard == 0xf1u &&
              noop.callbacks == std::vector<std::uint32_t>{0x5e0560u},
          "second nearby CRT wrapper uses the existing shared registration");
}

}  // namespace

int main() {
    try {
        StaticFields();
        LazyGlobal();
        CleanupAndCounter();
        std::cout << "script skill global record tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
