#include "pal4/actor_manager_combat_actor_binding.h"

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
constexpr std::uint32_t manager = 0x2000u;
constexpr std::uint32_t child = 0x3000u;

struct Runtime final : pal4::IActorManagerCombatBindingRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;
    bool valid = false;
    std::uint32_t created = child;

    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        trace.emplace_back("write:" + std::to_string(address) + ":" +
                           std::to_string(value));
        words[address] = value;
    }
    bool ProcessActionTask(std::uint32_t task) override {
        Check(task == 0x888u, "task value");
        trace.emplace_back("process_task");
        return valid;
    }
    void ActorSlot588(std::uint32_t address) override {
        Check(address == actor, "slot 588 actor");
        trace.emplace_back("slot588");
    }
    std::uint32_t ManagerSingleton() override {
        trace.emplace_back("manager");
        return manager;
    }
    std::uint32_t CreateChild(std::uint32_t collection,
                              std::uint32_t list,
                              std::string_view name) override {
        Check(collection == manager + 64u && list == manager + 80u &&
                  name == "hero", "create collection, list and name");
        trace.emplace_back("create");
        return created;
    }
    void GetAnimationFrame(std::uint32_t address) override {
        Check(address == child && words[actor + 808u] == child,
              "frame follows child pointer write");
        trace.emplace_back("frame");
    }
    void RenderCamera(std::uint32_t address, std::uint32_t first,
                      std::uint32_t second) override {
        Check(address == child && first == 0u && second == 0u,
              "camera arguments");
        trace.emplace_back("camera");
    }
    std::uint32_t InitializeChild(std::uint32_t address,
                                  std::uint32_t owner) override {
        Check(address == child && owner == actor, "child initializer args");
        trace.emplace_back("initialize");
        return 0x777u;
    }
    void ReleaseChild(std::uint32_t collection, std::uint32_t list,
                      std::uint32_t stride,
                      std::uint32_t address) override {
        Check(collection == manager + 64u && list == manager + 80u &&
                  stride == 12u && address == child &&
                  words[actor + 808u] == child,
              "release arguments and clear timing");
        trace.emplace_back("release");
    }
};

void Properties() {
    using pal4::ActorManagerCombatActorBinding;
    Runtime runtime;
    runtime.words[actor + 1156u] = 0x888u;
    runtime.valid = true;
    Check(ActorManagerCombatActorBinding::IsActionValid(actor, runtime) &&
              runtime.trace == std::vector<std::string>{
                  "read:5252", "process_task"},
          "action validation reads task at +1156");
    runtime.trace.clear();
    runtime.valid = false;
    Check(!ActorManagerCombatActorBinding::IsActionValid(actor, runtime),
          "false task result propagates");
    runtime.trace.clear();
    runtime.words[actor + 924u] = 0x1234u;
    Check(ActorManagerCombatActorBinding::Value924(
              actor, 0xdeadbeefu, runtime) == 0x1234u &&
              runtime.trace == std::vector<std::string>{"read:5020"},
          "getter reads +924 and ignores stack argument");
    runtime.trace.clear();
    Check(ActorManagerCombatActorBinding::SetValue1164(
              actor, 0x5678u, runtime) == 0x5678u &&
              runtime.words[actor + 1164u] == 0x5678u &&
              runtime.trace == std::vector<std::string>{"write:5260:22136"},
          "setter writes +1164 and returns value");
}

void AttachDetach() {
    using pal4::ActorManagerCombatActorBinding;
    Runtime success;
    Check(ActorManagerCombatActorBinding::AttachChild(
              actor, "hero", success) == 0x777u &&
              success.trace == std::vector<std::string>{
                  "slot588", "manager", "create", "write:4904:12288",
                  "frame", "camera", "initialize"},
          "attach stores child before ordered initialization");
    success.trace.clear();
    ActorManagerCombatActorBinding::DetachChild(actor, success);
    Check(success.words[actor + 808u] == 0u &&
              success.trace == std::vector<std::string>{
                  "read:4904", "manager", "release", "write:4904:0"},
          "detach releases collection entry before clearing child");
    success.trace.clear();
    ActorManagerCombatActorBinding::DetachChild(actor, success);
    Check(success.trace == std::vector<std::string>{"read:4904"},
          "detach skips manager when no child");

    Runtime failed;
    failed.created = 0u;
    Check(ActorManagerCombatActorBinding::AttachChild(
              actor, "hero", failed) == 0u &&
              failed.trace == std::vector<std::string>{
                  "slot588", "manager", "create", "write:4904:0"},
          "failed child creation stops after null pointer write");
}

}  // namespace

int main() {
    try {
        Properties();
        AttachDetach();
        std::cout << "actor_manager_combat_actor_binding_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
