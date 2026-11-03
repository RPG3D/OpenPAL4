#include "pal4/actor_manager_attack_events.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t actor = 0x1000u;
constexpr std::uint32_t target = 0x2000u;
constexpr std::uint32_t detail = 0x3000u;
constexpr std::uint32_t property = 0x4000u;

struct Runtime final : pal4::IActorManagerAttackEventRuntime {
    std::vector<std::string> trace;
    std::vector<std::uint32_t> payload;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == actor + 800u, "property field offset");
        trace.emplace_back("read-property");
        return property;
    }
    void Dispatch(std::string_view event,
                  std::span<const std::uint32_t> arguments,
                  std::uint32_t mode) override {
        Check(mode == 1u, "event mode");
        trace.emplace_back(event);
        payload.assign(arguments.begin(), arguments.end());
    }
    void InvokeProperty(std::uint32_t object,
                        std::uint32_t slot,
                        std::uint32_t subject,
                        std::uint32_t first,
                        std::uint32_t second) override {
        Check(object == property && subject == actor &&
              first == (slot >= 68u ? detail : target) && second == (slot >= 68u ? target : 0u),
              "property object and first argument");
        trace.push_back("slot:" + std::to_string(slot));
        trace.push_back("detail:" + std::to_string(second));
    }
};

using One = void (*)(std::uint32_t, std::uint32_t,
                     pal4::IActorManagerAttackEventRuntime&);
using Two = void (*)(std::uint32_t, std::uint32_t, std::uint32_t,
                     pal4::IActorManagerAttackEventRuntime&);

void OneArgumentRoutes() {
    constexpr std::array routes{
        std::pair<One, std::string_view>{
            &pal4::ActorManagerAttackEvents::PhysicalAttack,
            "ae_physical_attack"},
        std::pair<One, std::string_view>{
            &pal4::ActorManagerAttackEvents::CriticalAttack,
            "ae_critical_attack"},
        std::pair<One, std::string_view>{
            &pal4::ActorManagerAttackEvents::FendOffed,
            "ae_fend_offed"},
        std::pair<One, std::string_view>{
            &pal4::ActorManagerAttackEvents::AttackMissed,
            "ae_attack_missed"}};
    for (std::uint32_t i = 0; i < routes.size(); ++i) {
        Runtime runtime;
        routes[i].first(actor, 0u, runtime);
        Check(runtime.trace.empty(), "zero target suppresses all work");
        routes[i].first(actor, target, runtime);
        Check(runtime.payload == std::vector<std::uint32_t>{
                  actor + 716u, target} &&
              runtime.trace == std::vector<std::string>{
                  std::string(routes[i].second), "read-property",
                  "slot:" + std::to_string(52u + 4u * i), "detail:0"},
              "one-argument event payload and call order");
    }
}

void TwoArgumentRoutes() {
    constexpr std::array routes{
        std::pair<Two, std::string_view>{
            &pal4::ActorManagerAttackEvents::CastMagic,
            "ae_cast_magic"},
        std::pair<Two, std::string_view>{
            &pal4::ActorManagerAttackEvents::Stunt,
            "ae_stunt"},
        std::pair<Two, std::string_view>{
            &pal4::ActorManagerAttackEvents::UseProperty,
            "ae_use_property"}};
    for (std::uint32_t i = 0; i < routes.size(); ++i) {
        Runtime runtime;
        routes[i].first(actor, 0u, detail, runtime);
        routes[i].first(actor, target, 0u, runtime);
        Check(runtime.trace.empty(), "either zero argument suppresses work");
        routes[i].first(actor, target, detail, runtime);
        Check(runtime.payload == std::vector<std::uint32_t>{
                  actor + 716u, target, detail} &&
              runtime.trace == std::vector<std::string>{
                  std::string(routes[i].second), "read-property",
                  "slot:" + std::to_string(68u + 4u * i),
                  "detail:" + std::to_string(target)},
              "two-argument event payload and virtual call order");
    }
}

}  // namespace

int main() {
    try {
        OneArgumentRoutes();
        TwoArgumentRoutes();
        std::cout << "actor_manager_attack_events_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
