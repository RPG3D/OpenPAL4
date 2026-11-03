#include "pal4/actor_manager_property_events.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t actor = 0x1000u;
constexpr std::uint32_t context = 0x2000u;
constexpr std::uint32_t skill = 0x3000u;

struct Runtime final : pal4::IActorManagerPropertyEventRuntime {
    std::vector<std::string> trace;
    std::vector<std::uint32_t> payload;

    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == actor + 892u && value == 1u,
              "skill dirty flag offset and value");
        trace.emplace_back("dirty");
    }
    void AddSkillNative(std::uint32_t subject,
                        std::uint32_t native_context,
                        std::uint32_t native_skill) override {
        Check(subject == actor && native_context == context &&
              native_skill == skill, "add native arguments");
        trace.emplace_back("add-native");
    }
    void RemoveSkillNative(std::uint32_t subject,
                           std::uint32_t native_context,
                           std::uint32_t native_skill) override {
        Check(subject == actor && native_context == context &&
              native_skill == skill, "remove native arguments");
        trace.emplace_back("remove-native");
    }
    std::uint32_t Dispatch(std::string_view event,
                           std::span<const std::uint32_t> arguments,
                           std::uint32_t mode) override {
        Check(mode == 1u, "event dispatch mode");
        trace.emplace_back(event);
        payload.assign(arguments.begin(), arguments.end());
        return 0x4321u;
    }
    std::uint32_t CombatWounded(std::uint32_t subject,
                                std::uint32_t argument) override {
        Check(subject == actor && argument == 7u,
              "wounded arguments");
        trace.emplace_back("wounded");
        return 0x8800u;
    }
    std::uint32_t RegisterActorEvent(std::uint32_t event) override {
        Check(event == 0x8800u, "wounded result forwarded");
        trace.emplace_back("register");
        return 0x99u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == actor + 800u, "property pointer field");
        trace.emplace_back("read-property");
        return 0x7000u;
    }
    std::uint32_t InvokeProperty(std::uint32_t property,
                                 std::uint32_t slot,
                                 std::uint32_t subject,
                                 std::uint32_t first,
                                 std::uint32_t second,
                                 std::uint32_t third) override {
        Check(property == 0x7000u && subject == actor &&
              first == 1u && second == 2u && third == 3u,
              "property virtual argument order");
        trace.push_back("slot:" + std::to_string(slot));
        return slot + 100u;
    }
};

void SkillMutations() {
    Runtime runtime;
    Check(pal4::ActorManagerPropertyEvents::AddSkill(
              actor, context, 5u, skill, runtime) == 0x4321u &&
          runtime.trace == std::vector<std::string>{
              "dirty", "add-native", "ae_add_skill"} &&
          runtime.payload == std::vector<std::uint32_t>{
              actor + 716u, skill, 5u},
          "add skill native operation precedes event");
    runtime.trace.clear();
    Check(pal4::ActorManagerPropertyEvents::RemoveSkill(
              actor, context, skill, runtime) == 0x4321u &&
          runtime.trace == std::vector<std::string>{
              "dirty", "remove-native", "ae_remove_skill"} &&
          runtime.payload == std::vector<std::uint32_t>{
              actor + 716u, skill},
          "remove skill native operation precedes event");
}

void ImmunityEvents() {
    using Route = std::uint32_t (*)(
        std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t,
        pal4::IActorManagerPropertyEventRuntime&);
    constexpr std::array routes{
        std::pair<Route, std::string_view>{
            &pal4::ActorManagerPropertyEvents::ImmunityStateA,
            "ae_immunity_state_a"},
        std::pair<Route, std::string_view>{
            &pal4::ActorManagerPropertyEvents::ImmunityStateB,
            "ae_immunity_state_b"},
        std::pair<Route, std::string_view>{
            &pal4::ActorManagerPropertyEvents::ImmunitySkill,
            "ae_immunity_skill"}};
    for (const auto& [route, event] : routes) {
        Runtime runtime;
        Check(route(actor, 1u, 2u, 3u, runtime) == 0x4321u &&
              runtime.trace == std::vector<std::string>{
                  std::string(event)} &&
              runtime.payload == std::vector<std::uint32_t>{
                  actor + 716u, 2u, 1u, 3u},
              "immunity event key and payload word order");
    }
    Runtime runtime;
    Check(pal4::ActorManagerPropertyEvents::ImmunityNormal(
              actor, runtime) == 0x4321u &&
          runtime.trace == std::vector<std::string>{
              "ae_immunity_normal"} &&
          runtime.payload == std::vector<std::uint32_t>{actor + 716u},
          "normal immunity actor-only payload");
}

void WoundAndPropertyForwards() {
    Runtime runtime;
    Check(pal4::ActorManagerPropertyEvents::Wounded(
              actor, 7u, runtime) == 0x99u &&
          runtime.trace == std::vector<std::string>{
              "wounded", "register"},
          "wounded result feeds actor event registration");
    runtime.trace.clear();
    Check(pal4::ActorManagerPropertyEvents::ForwardSlot96(
              actor, 1u, 2u, 3u, runtime) == 196u &&
          pal4::ActorManagerPropertyEvents::ForwardSlot100(
              actor, 1u, 2u, 3u, runtime) == 200u &&
          pal4::ActorManagerPropertyEvents::ForwardSlot104(
              actor, 1u, 2u, 3u, runtime) == 204u &&
          pal4::ActorManagerPropertyEvents::ForwardSlot108(
              actor, 1u, 2u, 3u, runtime) == 208u &&
          runtime.trace == std::vector<std::string>{
              "read-property", "slot:96",
              "read-property", "slot:100",
              "read-property", "slot:104",
              "read-property", "slot:108"},
          "four property virtual forwards preserve args and return");
}

}  // namespace

int main() {
    try {
        SkillMutations();
        ImmunityEvents();
        WoundAndPropertyForwards();
        std::cout << "actor_manager_property_events_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
