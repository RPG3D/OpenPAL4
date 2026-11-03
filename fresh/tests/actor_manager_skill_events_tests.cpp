#include "pal4/actor_manager_skill_events.h"

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

struct Runtime final : pal4::IActorManagerSkillEventRuntime {
    std::vector<std::string> trace;
    std::vector<std::uint32_t> payload;

    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == 0x1000u + 892u && value == 1u,
              "dirty flag address and value");
        trace.emplace_back("write-dirty");
    }
    std::uint32_t Dispatch(std::string_view event,
                           std::span<const std::uint32_t> arguments,
                           std::uint32_t mode) override {
        Check(mode == 1u, "script event mode");
        trace.emplace_back(event);
        payload.assign(arguments.begin(), arguments.end());
        return 0x1234u;
    }
};

void RoutesAndEmptyCallbacks() {
    Runtime runtime;
    Check(pal4::ActorManagerSkillEvents::TriggerSkill(
              0x1000u, 1u, 2u, 3u, 4u, runtime) == 0x1234u &&
          runtime.trace == std::vector<std::string>{"ae_triger_skill"} &&
          runtime.payload == std::vector<std::uint32_t>{
              0x1000u + 716u, 1u, 4u, 3u, 2u},
          "skill event preserves five-word payload order");
    runtime.trace.clear();
    Check(pal4::ActorManagerSkillEvents::CallFunctionId(
              0x1000u, 1u, 2u, 3u, runtime) == 0x1234u &&
          runtime.trace == std::vector<std::string>{"ae_call_fun_id"} &&
          runtime.payload == std::vector<std::uint32_t>{
              0x1000u + 716u, 1u, 3u, 2u},
          "function-ID event preserves four-word payload order");
    runtime.trace.clear();
    Check(pal4::ActorManagerSkillEvents::RemoveMutexSkill(
              0x1000u, 9u, runtime) == 0x1234u &&
          runtime.trace == std::vector<std::string>{
              "write-dirty", "ae_remove_mutex_skill"} &&
          runtime.payload == std::vector<std::uint32_t>{
              0x1000u + 716u, 9u},
          "mutex skill marks actor before dispatch");
    runtime.trace.clear();
    pal4::ActorManagerSkillEvents::EmptyFourArgumentCallback(1u, 2u, 3u, 4u);
    pal4::ActorManagerSkillEvents::EmptyThreeArgumentCallback(1u, 2u, 3u);
    Check(runtime.trace.empty(), "empty callback entries have no effects");
}

}  // namespace

int main() {
    try {
        RoutesAndEmptyCallbacks();
        std::cout << "actor_manager_skill_events_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
