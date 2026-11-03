#include "pal4/world_manager_stage_events.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kManager = 0x1000u;
constexpr std::uint32_t kEventArgs = 0x2000u;
constexpr std::uint32_t kData = 0x3000u;
constexpr std::uint32_t kActor = 0x4000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IWorldManagerStageEventRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t dialog_data = kData;

    std::uint32_t DialogData(std::uint32_t event_args) override {
        Check(event_args == kEventArgs, "event argument forwarded");
        calls.emplace_back("dialog-data");
        return dialog_data;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void ActivateActor(std::uint32_t actor,
                       std::uint32_t camera_argument,
                       std::uint32_t actor_data) override {
        Check(actor == kActor && camera_argument == 0x5000u &&
                  actor_data == kManager + 328u,
              "join actor, camera and actor-data arguments");
        calls.emplace_back("activate");
    }
    void LeaveActor(std::uint32_t actor,
                    const std::array<std::uint32_t, 3>& values,
                    std::uint32_t mode) override {
        Check(actor == kActor &&
                  values == std::array<std::uint32_t, 3>{
                      0u, 0x7f7fffffu, 0u} && mode == 0u,
              "leave virtual-slot payload and mode");
        calls.emplace_back("leave");
    }
};

void JoinAndWrapper() {
    Runtime runtime;
    runtime.words[kData + 808u] = kActor;
    runtime.words[kManager + 132u] = 0x5000u;
    pal4::WorldManagerStageEvents::JoinWrapper(
        kManager, kEventArgs, runtime);
    Check(runtime.calls == (std::vector<std::string>{
              "dialog-data", "read:13096", "read:4228", "activate"}),
          "join wrapper forwards to handler in IDA order");

    Runtime absent_actor;
    pal4::WorldManagerStageEvents::Join(
        kManager, kEventArgs, absent_actor);
    Check(absent_actor.calls.size() == 2u,
          "join ignores absent actor without reading manager camera");

    Runtime absent_data;
    absent_data.dialog_data = 0u;
    pal4::WorldManagerStageEvents::Join(
        kManager, kEventArgs, absent_data);
    Check(absent_data.calls == std::vector<std::string>{"dialog-data"},
          "missing event data has defined no-op behavior");
}

void LeaveAndWrapper() {
    Runtime runtime;
    runtime.words[kData + 808u] = kActor;
    pal4::WorldManagerStageEvents::LeaveWrapper(
        kManager, kEventArgs, runtime);
    Check(runtime.calls == (std::vector<std::string>{
              "dialog-data", "read:13096", "leave"}),
          "leave wrapper invokes actor virtual-slot operation");

    Runtime absent_actor;
    pal4::WorldManagerStageEvents::Leave(
        kManager, kEventArgs, absent_actor);
    Check(absent_actor.calls.size() == 2u,
          "leave ignores absent actor");
}

}  // namespace

int main() {
    try {
        JoinAndWrapper();
        LeaveAndWrapper();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
