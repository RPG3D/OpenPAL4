#include "pal4/actor_manager_action_events.h"

#include <cstdint>
#include <iostream>
#include <map>
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

struct Runtime final : pal4::IActorManagerActionEventRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;
    std::vector<float> conversions;
    std::uint32_t state = 3u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::int32_t ReadMetric(std::uint32_t source,
                            std::uint32_t other,
                            std::uint32_t slot) override {
        Check(source == actor && other == target &&
              (slot == 428u || slot == 432u),
              "metric virtual slot and target");
        trace.push_back("metric:" + std::to_string(slot));
        return slot == 432u ? 11 : 7;
    }
    std::int64_t FloatToInteger(float value) override {
        conversions.push_back(value);
        trace.push_back("ftol");
        return value > 4.0f ? 6 : 4;
    }
    std::uint32_t AddMetric8(std::uint32_t subject,
                             std::uint32_t value) override {
        Check((subject == actor && value == 6u) ||
              (subject == target && value == 4u),
              "metric eight target and converted value");
        trace.push_back("set:" + std::to_string(subject));
        return subject == target ? 0x88u : 0x77u;
    }
    void HandleEvent14(std::uint32_t subject,
                       std::uint32_t selector,
                       std::uint32_t flag) override {
        Check(subject == actor && flag == 1u &&
              (selector == 1u || selector == 2u),
              "action handler selector and flag");
        trace.push_back("event14:" + std::to_string(selector));
    }
    std::uint32_t Dispatch(std::string_view event,
                           std::uint32_t subject,
                           std::uint32_t mode) override {
        Check(subject == actor && mode == 1u,
              "action event actor and mode");
        trace.push_back(std::string(event));
        return 0x99u;
    }
    void InvokeActorSlot396(std::uint32_t subject,
                            std::uint32_t argument) override {
        Check(subject == actor && argument == 1u,
              "end action slot +396 argument");
        trace.push_back("slot396");
    }
    std::uint32_t InvokePropertySlot(
        std::uint32_t property, std::uint32_t slot,
        std::uint32_t subject, std::uint32_t argument) override {
        Check(property == 0x7000u && subject == actor &&
              (argument == 0u || argument == target),
              "property callback pointer and arguments");
        trace.push_back("property:" + std::to_string(slot));
        return slot + 100u;
    }
    std::uint32_t InvokeActorSlot144(std::uint32_t subject) override {
        Check(subject == actor, "state slot +144 actor");
        trace.push_back("slot144");
        return state;
    }
    std::uint32_t InvokeActorSlot148(std::uint32_t subject,
                                     std::uint32_t argument) override {
        Check(subject == actor && argument == 0u,
              "reset slot +148 actor and zero argument");
        trace.push_back("slot148");
        return 0x102u;
    }
};

void MetricTransfer() {
    Runtime runtime;
    runtime.words[actor + 804u] = 10u;
    runtime.words[target + 804u] = 10u;
    Check(pal4::ActorManagerActionEvents::TransferMetric8(
              actor, target, 0.5f, runtime) == 1u &&
          runtime.trace.empty(),
          "same owner returns check result without metric calls");
    runtime.words[target + 804u] = 11u;
    Check(pal4::ActorManagerActionEvents::TransferMetric8(
              actor, target, 0.5f, runtime) == 0x88u &&
          runtime.conversions == std::vector<float>{5.5f, 3.5f} &&
          runtime.trace == std::vector<std::string>{
              "metric:432", "metric:428", "ftol", "set:4096",
              "ftol", "set:8192"},
          "distinct owner scales both metrics then sets actor and target");
}

void ActionEventsAndReset() {
    Runtime runtime;
    runtime.words[actor + 800u] = 0x7000u;
    runtime.words[actor + 804u] = 10u;
    runtime.words[target + 804u] = 11u;
    Check(pal4::ActorManagerActionEvents::StartAction(
              actor, runtime) == 128u &&
          runtime.trace == std::vector<std::string>{
              "event14:2", "ae_start_action", "property:28"},
          "start action dispatches before property slot +28");
    runtime.trace.clear();
    runtime.words[actor + 828u] = 2u;
    Check(pal4::ActorManagerActionEvents::EndAction(
              actor, runtime) == 132u &&
          runtime.trace == std::vector<std::string>{
              "event14:1", "slot396", "ae_end_action", "property:32"},
          "end state two handles event and actor slot before dispatch");
    runtime.trace.clear();
    runtime.words[actor + 828u] = 1u;
    Check(pal4::ActorManagerActionEvents::EndAction(
              actor, runtime) == 132u &&
          runtime.trace == std::vector<std::string>{
              "ae_end_action", "property:32"},
          "other end states skip special handler");
    runtime.trace.clear();
    runtime.words[target + 804u] = 10u;
    Check(pal4::ActorManagerActionEvents::ResetForTarget(
              actor, target, runtime) == 1u && runtime.trace.empty(),
          "same owner skips reset state virtuals");
    runtime.words[target + 804u] = 11u;
    Check(pal4::ActorManagerActionEvents::ResetForTarget(
              actor, target, runtime) == 3u &&
          runtime.trace == std::vector<std::string>{"slot144"},
          "non-four state returns slot +144 low byte");
    runtime.state = 4u;
    runtime.trace.clear();
    Check(pal4::ActorManagerActionEvents::ResetForTarget(
              actor, target, runtime) == 2u &&
          runtime.trace == std::vector<std::string>{
              "slot144", "slot148"},
          "state four forwards zero to slot +148 and truncates result");
    runtime.trace.clear();
    Check(pal4::ActorManagerActionEvents::ForwardSlot36(
              actor, target, runtime) == 136u &&
          pal4::ActorManagerActionEvents::ForwardSlot40(
              actor, target, runtime) == 140u &&
          pal4::ActorManagerActionEvents::ForwardSlot44(
              actor, target, runtime) == 144u &&
          pal4::ActorManagerActionEvents::ForwardSlot48(
              actor, target, runtime) == 148u &&
          runtime.trace == std::vector<std::string>{
              "slot144", "slot148", "property:36",
              "slot144", "slot148", "property:40",
              "slot144", "slot148", "property:44",
              "slot144", "slot148", "property:48"},
          "four wrappers reset actor then call exact property slots");
}

}  // namespace

int main() {
    try {
        MetricTransfer();
        ActionEventsAndReset();
        std::cout << "actor_manager_action_events_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
