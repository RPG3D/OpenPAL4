#include "pal4/combat_event_callbacks.h"

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

struct Runtime final : pal4::ICombatEventCallbackRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t remaining = 2u;
    bool fail_actor = false;
    bool fail_hint = false;

    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte:" + std::to_string(address));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint32_t BeginActors(std::uint32_t collection) override {
        calls.emplace_back("begin:" + std::to_string(collection));
        return 0x3000u;
    }
    bool AtEnd(std::uint32_t iterator) override {
        Check(iterator == 0x3000u, "iterator identity");
        return remaining == 0u;
    }
    std::uint32_t Actor(std::uint32_t iterator) override {
        Check(iterator == 0x3000u, "actor iterator identity");
        return 0x4000u + remaining;
    }
    void ProcessActor(std::uint32_t actor) override {
        calls.emplace_back("actor:" + std::to_string(actor));
        if (fail_actor) throw std::runtime_error("actor");
    }
    void Advance(std::uint32_t iterator) override {
        Check(iterator == 0x3000u, "advance iterator identity");
        calls.emplace_back("advance");
        --remaining;
    }
    void DestroyIterator(std::uint32_t iterator) override {
        Check(iterator == 0x3000u, "destroy iterator identity");
        calls.emplace_back("destroy");
    }
    std::uint32_t CombatSystemInstance() override {
        calls.emplace_back("system");
        return 0x5000u;
    }
    std::uint32_t GameString(std::uint32_t identifier) override {
        Check(identifier == 0x100u, "game string key");
        calls.emplace_back("game-string");
        return 0x6000u;
    }
    std::uint32_t FormatHint(std::uint32_t format,
                             std::uint32_t value) override {
        Check(format == 0x6000u, "format string identity");
        calls.emplace_back("format:" + std::to_string(value));
        return 0x7000u;
    }
    void CreateHintWindow() override {
        calls.emplace_back("window");
    }
    void ShowHint(std::uint32_t text, float x, float y,
                  float duration, std::uint32_t first,
                  std::uint32_t second) override {
        Check(text == 0x7000u && x == 0.5f &&
                  y > 0.199f && y < 0.201f && duration == 4.0f &&
                  first == 0u && second == 0u,
              "hint layout and duration");
        calls.emplace_back("show");
        if (fail_hint) throw std::runtime_error("hint");
    }
    std::uint32_t ReleaseHint(std::uint32_t text) override {
        Check(text == 0x7000u, "hint cleanup identity");
        calls.emplace_back("release");
        return 0x55u;
    }
};

void EventBranches() {
    for (std::uint32_t type : {1u, 2u}) {
        Runtime runtime;
        runtime.words[0x1000u] = type;
        runtime.words[0x5000u + 440u] = 3u;
        Check(pal4::CombatEventCallbacks::HandleEvent(
                  0x2000u, 0x1000u, runtime) == 0x55u &&
                  runtime.bytes[0x2000u + 136u] == 0u &&
                  runtime.calls == std::vector<std::string>{
                      "byte:8328", "begin:" + std::to_string(type),
                      "actor:16386", "advance", "actor:16385",
                      "advance", "destroy", "system", "game-string",
                      "format:3", "window", "show", "release"},
              "both actor collections process then show formatted hint");
    }

    Runtime other;
    other.words[0x1000u] = 7u;
    other.words[0x5000u + 440u] = 1u;
    Check(pal4::CombatEventCallbacks::HandleEvent(
              0x2000u, 0x1000u, other) == 0x5000u &&
              other.calls == std::vector<std::string>{
                  "byte:8328", "system"},
          "unhandled event and low property skip actor scan and hint");
    other.words[0x5000u + 440u] = 0xffffffffu;
    other.calls.clear();
    Check(pal4::CombatEventCallbacks::HandleEvent(
              0x2000u, 0x1000u, other) == 0x5000u,
          "negative property does not show hint");
}

void CleanupOnFailure() {
    Runtime actor;
    actor.words[0x1000u] = 1u;
    actor.fail_actor = true;
    bool raised = false;
    try {
        static_cast<void>(pal4::CombatEventCallbacks::HandleEvent(
            0x2000u, 0x1000u, actor));
    } catch (const std::runtime_error&) {
        raised = true;
    }
    Check(raised && actor.calls.back() == "destroy",
          "actor failure destroys iterator");

    Runtime hint;
    hint.words[0x1000u] = 0u;
    hint.words[0x5000u + 440u] = 2u;
    hint.fail_hint = true;
    raised = false;
    try {
        static_cast<void>(pal4::CombatEventCallbacks::HandleEvent(
            0x2000u, 0x1000u, hint));
    } catch (const std::runtime_error&) {
        raised = true;
    }
    Check(raised && hint.calls.back() == "release",
          "hint failure releases formatted text");
}

void ResetOnly() {
    Runtime runtime;
    runtime.bytes[0x2000u + 136u] = 1u;
    pal4::CombatEventCallbacks::ResetState(0x2000u, runtime);
    Check(runtime.bytes[0x2000u + 136u] == 0u &&
              runtime.calls == std::vector<std::string>{"byte:8328"},
          "reset callback only clears state byte");
}

}  // namespace

int main() {
    try {
        EventBranches();
        CleanupOnFailure();
        ResetOnly();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
