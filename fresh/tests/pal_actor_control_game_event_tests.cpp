#include "pal4/pal_actor_control_interaction.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime : pal4::IActorControlInteractionRuntime {
    std::vector<std::string> events;
    std::int32_t event = 1;
    std::uint32_t window = 0x600u;
    std::optional<std::string> text = std::string("hint text");
    std::string expected_text = "hint text";
    std::uint32_t encoded_size = 9u;

    std::int32_t EventCode(std::uint32_t dialog) override {
        Check(dialog == 0x700u, "event dialog address");
        events.emplace_back("code");
        return event;
    }
    std::uint32_t CreateHintWindow() override {
        events.emplace_back("window");
        return window;
    }
    std::optional<std::string> TriggerText(std::uint32_t id) override {
        Check(id == (event == 1 ? 0x109u : 0x10au),
              "game database trigger id");
        events.emplace_back("text");
        return text;
    }
    std::string ConvertToUtf8(std::string_view value) override {
        Check(value == expected_text, "UTF8 source text");
        events.emplace_back("utf8");
        return std::string(value);
    }
    std::string AssignHintText(std::string_view value) override {
        Check(event == 1 && value == expected_text,
              "event one CEGUI assign");
        events.emplace_back("assign");
        return std::string(value);
    }
    std::uint32_t EncodedSize(std::string_view value) override {
        Check(event == 3 && value == expected_text,
              "event three CEGUI encoded size");
        events.emplace_back("size");
        return encoded_size;
    }
    std::string EncodeHintText(std::string_view value,
                               std::uint32_t size) override {
        Check(event == 3 && value == expected_text && size == 9u,
              "event three CEGUI encoding");
        events.emplace_back("encode");
        return std::string(value);
    }
    void InitializeHintWindow(
        std::uint32_t target, std::string_view value,
        float x, float y, float opacity, std::uint32_t zero,
        std::uint32_t one) override {
        Check(target == 0x600u && value == expected_text &&
                  x == 0.5f && y == 0.15f && opacity == 1.0f &&
                  zero == 0u && one == 1u,
              "hint window layout arguments");
        events.emplace_back("initialize");
    }
    void PlayHintSound(std::string_view path,
                       std::uint32_t repeat) override {
        Check(path == pal4::PalActorControlGameEvent::kSoundPath &&
                  repeat == 1u, "hint sound arguments");
        events.emplace_back("sound");
    }
};

void TestGates() {
    pal4::PalActorControlState state{};
    Runtime runtime;
    runtime.event = -1;
    Check(pal4::PalActorControlGameEvent::Process(
              state, 0x700u, runtime) &&
              runtime.events == std::vector<std::string>{"code"},
          "negative event skips UI and returns true");
    runtime.events.clear();
    runtime.event = 3;
    state.words[19] = 3u;
    Check(pal4::PalActorControlGameEvent::Process(
              state, 0x700u, runtime) &&
              runtime.events == std::vector<std::string>{"code"},
          "matching controller field skips UI");
    runtime.events.clear();
    runtime.event = 2;
    state.words[19] = 0u;
    Check(pal4::PalActorControlGameEvent::Process(
              state, 0x700u, runtime) &&
              runtime.events == std::vector<std::string>{"code"},
          "other positive event returns true");
    runtime.events.clear();
    runtime.event = 1;
    runtime.window = 0u;
    Check(!pal4::PalActorControlGameEvent::Process(
              state, 0x700u, runtime) &&
              runtime.events == std::vector<std::string>{
                  "code", "window"},
          "missing hint window returns false without database lookup");
}

void TestTextBranches() {
    pal4::PalActorControlState state{};
    Runtime runtime;
    runtime.event = 1;
    Check(!pal4::PalActorControlGameEvent::Process(
              state, 0x700u, runtime) &&
              runtime.events == std::vector<std::string>{
                  "code", "window", "text", "utf8", "assign",
                  "initialize", "sound"},
          "event one assigns CEGUI text and plays hint sound");
    runtime.events.clear();
    runtime.event = 3;
    Check(!runtime.ProcessDialogEvent(state, 0x700u) &&
              runtime.events == std::vector<std::string>{
                  "code", "window", "text", "utf8", "size",
                  "encode", "initialize", "sound"},
          "default interaction binding uses event-three encoded path");
    runtime.events.clear();
    runtime.encoded_size = std::numeric_limits<std::uint32_t>::max();
    try {
        static_cast<void>(pal4::PalActorControlGameEvent::Process(
            state, 0x700u, runtime));
        Check(false, "invalid encoded size must throw");
    } catch (const std::length_error&) {
        Check(runtime.events == std::vector<std::string>{
                  "code", "window", "text", "utf8", "size"},
              "length error occurs before hint or audio calls");
    }
    runtime = {};
    runtime.text = std::nullopt;
    runtime.expected_text.clear();
    Check(!pal4::PalActorControlGameEvent::Process(
              state, 0x700u, runtime) &&
              runtime.events.back() == "sound",
          "missing database string becomes an empty hint");
    runtime = {};
    runtime.event = 3;
    runtime.text = std::string("hint text\0tail", 14u);
    Check(!pal4::PalActorControlGameEvent::Process(
              state, 0x700u, runtime) &&
              runtime.events.back() == "sound",
          "source text stops at NUL before UTF8 conversion");
}

}  // namespace

int main() {
    TestGates();
    TestTextBranches();
}
