#include "pal4/actor_animation_events.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IActorAnimationEventRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> texts;
    std::array<std::uint32_t, 2> last_payload{};
    std::uint32_t last_router = 0;
    std::uint32_t last_key = 0;
    std::uint32_t last_mode = 0;
    std::uint32_t route_calls = 0;
    std::uint32_t compare_calls = 0;
    std::uint32_t dispatch_result = 0x77u;
    bool found = false;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    std::string_view TextAt(const std::uint32_t address) override {
        return texts.at(address);
    }
    bool EqualIgnoringCase(const std::string_view left,
                           const std::string_view right) override {
        ++compare_calls;
        return left.size() == right.size() &&
               std::equal(left.begin(), left.end(), right.begin(),
                   [](const char a, const char b) {
                       return std::tolower(static_cast<unsigned char>(a)) ==
                              std::tolower(static_cast<unsigned char>(b));
                   });
    }
    void SetAnimationFound(const bool value) override { found = value; }
    std::uint32_t EventRouter() override {
        ++route_calls;
        return 0x3000u;
    }
    std::uint32_t Dispatch(
        const std::uint32_t router, const std::uint32_t event_key,
        const std::array<std::uint32_t, 2>& payload,
        const std::uint32_t mode) override {
        last_router = router;
        last_key = event_key;
        last_payload = payload;
        last_mode = mode;
        return dispatch_result;
    }
};

void NameComparison() {
    Runtime runtime;
    constexpr auto object = 0x1000u;
    constexpr auto search = 0x2000u;
    Check(pal4::ActorAnimationEvents::CompareNameAndFlag(
              object, search, runtime) == 0u && runtime.found &&
          runtime.compare_calls == 1u,
          "two null string pointers compare as empty and mark found");
    runtime.found = false;
    runtime.words[object + 20u] = 0x5000u;
    runtime.words[search + 4u] = 0x6000u;
    runtime.texts[0x5000u] = "RunLoop";
    runtime.texts[0x6000u] = "runloop";
    Check(pal4::ActorAnimationEvents::CompareNameAndFlag(
              object, search, runtime) == 0u && runtime.found,
          "case-insensitive animation name match sets flag");
    runtime.found = false;
    runtime.texts[0x6000u] = "walk";
    Check(pal4::ActorAnimationEvents::CompareNameAndFlag(
              object, search, runtime) == object && !runtime.found,
          "mismatch returns original object without setting flag");
}

void MessageDispatch() {
    Runtime runtime;
    constexpr auto object = 0x1000u;
    Check(pal4::ActorAnimationEvents::ProcessMessage(
              object, 0xdeadbeefu, 9u, runtime) == 1u &&
          runtime.route_calls == 0u,
          "missing actor returns one without touching event router");
    runtime.words[object + 4u] = 0x5000u;
    runtime.words[0x8b595cu] = 0x7000u;
    Check(pal4::ActorAnimationEvents::ProcessMessage(
              object, 0xdeadbeefu, 9u, runtime) == 0x77u &&
          runtime.route_calls == 1u &&
          runtime.last_router == 0x3000u &&
          runtime.last_key == 0x7000u &&
          runtime.last_payload ==
              std::array<std::uint32_t, 2>{0x5000u + 716u, 9u} &&
          runtime.last_mode == 1u,
          "animation event carries actor +716, message and routing mode");
}

}  // namespace

int main() {
    try {
        NameComparison();
        MessageDispatch();
        std::cout << "actor_animation_events_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
