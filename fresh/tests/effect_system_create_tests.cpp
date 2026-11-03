#include "pal4/effect_system.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, std::string_view message) {
    if (condition) return;
    std::cerr << message << '\n';
    std::exit(1);
}

struct Runtime final : pal4::IEffectSystemCreateRuntime {
    static constexpr std::uint32_t kSystem = 0x1000u;
    static constexpr std::uint32_t kEffect = 0x2000u;
    static constexpr std::uint32_t kPosition = 0x3000u;
    static constexpr std::uint32_t kRepeatCount = 3u;
    bool enabled = true;
    bool uppercase = false;
    bool acquired = true;
    bool started = true;
    bool paused = true;
    bool existing_name = false;
    bool existing_id = false;
    bool throw_on_notify = false;
    std::uint32_t next_id = 7u;
    std::vector<std::string> calls;

    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == kSystem + 4u, "enabled byte address");
        calls.emplace_back("enabled");
        return enabled ? 1u : 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == kSystem + 8u, "next id address");
        calls.emplace_back("read id");
        return next_id;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == kEffect + 83u && value == 0x5au,
              "effect playback flag");
        calls.emplace_back("flag");
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        if (address == kEffect + 12u) {
            Check(value == kSystem, "effect parent");
            calls.emplace_back("parent");
        } else {
            Check(address == kSystem + 8u && value == next_id + 1u,
                  "incremented id");
            next_id = value;
            calls.emplace_back("write id");
        }
    }
    void BeginScriptScope() override { calls.emplace_back("begin"); }
    void EndScriptScope() override { calls.emplace_back("end"); }
    bool UppercaseNames() override {
        calls.emplace_back("uppercase?");
        return uppercase;
    }
    std::uint32_t AcquireEffect(std::string_view name) override {
        calls.emplace_back("acquire " + std::string(name));
        return acquired ? kEffect : 0u;
    }
    void ReturnEffect(std::uint32_t effect,
                      std::uint8_t mode) override {
        Check(effect == kEffect && mode == 1u, "return to pool mode");
        calls.emplace_back("return");
    }
    std::uint32_t FindXmlAttribute(std::uint32_t xml,
                                    std::string_view key) override {
        Check(xml == kEffect + 20u, "effect XML address");
        calls.emplace_back("find " + std::string(key));
        return key == "NAME" ? (existing_name ? 0x5000u : 0u)
                             : (existing_id ? 0x6000u : 0u);
    }
    void SetXmlString(std::uint32_t attribute,
                      std::string_view value) override {
        Check(attribute == 0x5000u, "existing NAME attribute");
        calls.emplace_back("set name " + std::string(value));
    }
    void AddXmlString(std::uint32_t xml, std::string_view key,
                      std::string_view value) override {
        Check(xml == kEffect + 20u && key == "NAME", "new NAME attribute");
        calls.emplace_back("add name " + std::string(value));
    }
    void SetXmlInt(std::uint32_t attribute,
                   std::uint32_t value) override {
        Check(attribute == 0x6000u && value == next_id - 1u,
              "existing ID attribute");
        calls.emplace_back("set id");
    }
    void AddXmlInt(std::uint32_t xml, std::string_view key,
                   std::uint32_t value) override {
        Check(xml == kEffect + 20u && key == "ID" &&
              value == next_id - 1u, "new ID attribute");
        calls.emplace_back("add id");
    }
    void SetPosition(std::uint32_t effect, std::uint32_t position,
                     std::uint8_t mode) override {
        Check(effect == kEffect && position == kPosition && mode == 0u,
              "effect initial position");
        calls.emplace_back("position");
    }
    bool StartPlayback(std::uint32_t effect) override {
        Check(effect == kEffect, "start effect");
        calls.emplace_back("start");
        return started;
    }
    bool PausePlayback(std::uint32_t effect) override {
        Check(effect == kEffect, "pause effect");
        calls.emplace_back("pause");
        return paused;
    }
    void InsertEffect(std::uint32_t tree, std::uint32_t id,
                      std::uint32_t effect,
                      std::uint32_t repeat_count) override {
        Check(tree == kSystem + 12u && id == next_id - 1u &&
              effect == kEffect && repeat_count == kRepeatCount,
              "effect tree insertion");
        calls.emplace_back("insert");
    }
    pal4::EffectSystemHandle CopyHandle(
        std::uint32_t id, std::uint32_t system) override {
        Check(id == next_id - 1u && system == kSystem,
              "effect handle source");
        calls.emplace_back("copy handle");
        return {id, system};
    }
    void ResetTemporaryHandle(pal4::EffectSystemHandle& handle) override {
        Check(handle == pal4::EffectSystemHandle{next_id - 1u, kSystem},
              "temporary effect handle");
        calls.emplace_back("reset handle");
        handle = {};
    }
    void NotifyCreated(std::uint32_t system,
                       const pal4::EffectSystemHandle& handle,
                       std::uint32_t position,
                       std::uint32_t repeat_count) override {
        Check(system == kSystem &&
              handle == pal4::EffectSystemHandle{next_id - 1u, kSystem} &&
              position == kPosition && repeat_count == kRepeatCount,
              "creation callback arguments");
        calls.emplace_back("notify");
        if (throw_on_notify) throw std::runtime_error("callback");
    }
};

pal4::EffectSystemHandle Create(Runtime& runtime, bool pause) {
    return pause
        ? pal4::EffectSystem::CreateEffectObject(
              Runtime::kSystem, "spark", Runtime::kPosition,
              Runtime::kRepeatCount, 0x5au, runtime)
        : pal4::EffectSystem::CreateEffectObjectWithoutPause(
              Runtime::kSystem, "spark", Runtime::kPosition,
              Runtime::kRepeatCount, 0x5au, runtime);
}

void TestGatesAndFailures() {
    // Compare with the original wire values, not another default-constructed
    // value: both would otherwise hide a wrong invalid-handle initializer.
    constexpr pal4::EffectSystemHandle invalid{0xffffffffu, 0u};
    for (const bool pause : {true, false}) {
        Runtime inactive;
        inactive.enabled = false;
        Check(Create(inactive, pause) == invalid &&
              inactive.calls == std::vector<std::string>{"enabled"},
              "inactive system performs no creation work");

        Runtime missing;
        missing.acquired = false;
        Check(Create(missing, pause) == invalid &&
              missing.calls == (std::vector<std::string>{
                  "enabled", "begin", "uppercase?", "acquire spark", "end"}),
              "pool miss returns an empty handle and closes scope");

        Runtime start_failure;
        start_failure.started = false;
        Check(Create(start_failure, pause) == invalid &&
              start_failure.next_id == 7u &&
              start_failure.calls == (std::vector<std::string>{
                  "enabled", "begin", "uppercase?", "acquire spark",
                  "find NAME", "add name spark", "parent", "position",
                  "flag", "start", "return", "end"}),
              "start failure returns object before assigning an id");
    }

    Runtime pause_failure;
    pause_failure.paused = false;
    Check(Create(pause_failure, true) == invalid &&
          pause_failure.next_id == 7u &&
          pause_failure.calls == (std::vector<std::string>{
              "enabled", "begin", "uppercase?", "acquire spark",
              "find NAME", "add name spark", "parent", "position",
              "flag", "start", "pause", "return", "end"}),
          "paused variant returns object when pause fails");
}

void TestSuccessfulPaths() {
    Runtime paused;
    paused.uppercase = true;
    paused.existing_name = true;
    paused.existing_id = true;
    Check(Create(paused, true) ==
              pal4::EffectSystemHandle{7u, Runtime::kSystem} &&
          paused.next_id == 8u &&
          paused.calls == (std::vector<std::string>{
              "enabled", "begin", "uppercase?", "acquire SPARK",
              "find NAME", "set name SPARK", "parent", "position",
              "flag", "start", "pause", "read id", "write id",
              "insert", "find ID", "set id", "copy handle",
              "copy handle", "notify", "reset handle", "reset handle",
              "end"}),
          "paused path updates existing attributes before both handle copies");

    Runtime immediate;
    Check(Create(immediate, false) ==
              pal4::EffectSystemHandle{7u, Runtime::kSystem} &&
          immediate.calls == (std::vector<std::string>{
              "enabled", "begin", "uppercase?", "acquire spark",
              "find NAME", "add name spark", "parent", "position",
              "flag", "start", "read id", "write id", "insert",
              "copy handle", "find ID", "add id", "copy handle",
              "notify", "reset handle", "reset handle", "end"}),
          "immediate path skips pause and sets ID after first handle copy");
}

void TestCallbackExceptionCleanup() {
    Runtime runtime;
    runtime.throw_on_notify = true;
    bool thrown = false;
    try {
        (void)Create(runtime, true);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && runtime.calls.size() >= 4u &&
          runtime.calls[runtime.calls.size() - 3u] == "reset handle" &&
          runtime.calls[runtime.calls.size() - 2u] == "reset handle" &&
          runtime.calls.back() == "end",
          "callback exception releases both temporary handles and script scope");
}

}  // namespace

int main() {
    TestGatesAndFailures();
    TestSuccessfulPaths();
    TestCallbackExceptionCleanup();
}
