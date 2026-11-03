#include "pal4/gob_obstacle_combat_callback.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGobObstacleCombatCallbackRuntime {
    static constexpr std::uint32_t callback = 0x1000u;
    static constexpr std::uint32_t obstacle = 0x2000u;
    static constexpr std::uint32_t movement = 0x3000u;
    static constexpr std::uint32_t movement_root = 0x4000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::array<std::uint32_t, 3> data{1u, 1u, 1u};
    std::uint32_t effect_context = 0u;

    Runtime() {
        words[callback + 8u] = obstacle;
        words[movement] = movement_root;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write " + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t MovementAdapter() override {
        calls.emplace_back("movement");
        return movement;
    }
    void InvokeObjectSlot80(
        std::uint32_t object,
        const std::array<std::uint32_t, 3>& incoming) override {
        Check(object == obstacle, "slot 80 object");
        calls.emplace_back("slot 80");
        data = incoming;
    }
    void ApplyEffectTypeToActor(
        std::uint32_t object, std::uint32_t context) override {
        Check(object == obstacle, "effect type object");
        calls.emplace_back("effect");
        effect_context = context;
    }
};

void CheckConstructors() {
    Runtime derived;
    Check(pal4::GobObstacleCombatCallback::Construct(
              Runtime::callback, derived) == Runtime::callback &&
          derived.words.at(Runtime::callback) == 0x8493acu &&
          derived.words.at(Runtime::callback + 4u) == 0u &&
          derived.words.at(Runtime::callback + 8u) == 0u &&
          derived.calls == (std::vector<std::string>{
              "write 4100", "write 4096", "write 4104"}),
          "obstacle callback constructor and write order");

    Runtime base;
    Check(pal4::GobObstacleCombatCallback::ConstructBase(
              Runtime::callback, base) == 0u &&
          base.words.at(Runtime::callback) == 0x84932cu &&
          base.words.at(Runtime::callback + 4u) == 0u &&
          base.words.at(Runtime::callback + 8u) == 0u &&
          base.calls == (std::vector<std::string>{
              "write 4096", "write 4104", "write 4100"}),
          "abstract callback base constructor returns zero");
}

void CheckProcess() {
    Runtime gated;
    Check(pal4::GobObstacleCombatCallback::Process(
              Runtime::callback, 0x7777u, 1u, gated) == 0u &&
          gated.calls.empty(),
          "nonzero argument disables obstacle callback");

    Runtime empty;
    empty.words[Runtime::callback + 8u] = 0u;
    Check(pal4::GobObstacleCombatCallback::Process(
              Runtime::callback, 0x7777u, 0u, empty) == 1u &&
          empty.calls.empty(),
          "null obstacle still returns success");

    Runtime active;
    Check(pal4::GobObstacleCombatCallback::Process(
              Runtime::callback, 0x7777u, 0u, active) == 1u &&
          active.data == (std::array<std::uint32_t, 3>{0u, 0u, 0u}) &&
          active.effect_context == 0x7777u &&
          active.words.at(Runtime::movement_root + 88u) == 1u &&
          active.calls == (std::vector<std::string>{
              "slot 80", "effect", "movement", "write 16472"}),
          "obstacle callback dispatch and movement flag order");
}

}  // namespace

int main() {
    try {
        CheckConstructors();
        CheckProcess();
        std::cout << "gob_obstacle_combat_callback_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
