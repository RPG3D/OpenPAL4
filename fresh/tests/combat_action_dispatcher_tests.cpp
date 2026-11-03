#include "pal4/combat_action_dispatcher.h"

#include <algorithm>
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

struct TaskCall {
    pal4::CombatActionKind kind;
    std::uint32_t resource;
    std::uint32_t actor;
    std::uint32_t target;
    std::uint32_t auxiliary;
};

struct Runtime final : pal4::ICombatActionDispatchRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<TaskCall> tasks;
    std::vector<std::uint32_t> dynamic_targets;
    std::size_t target_index = 0u;
    std::uint32_t lookup_result = 0x55u;
    std::uint32_t next_task = 0x6000u;

    std::uint32_t ActorFromFacet(std::uint32_t event) override {
        Check(event == 0x1000u, "facet event address");
        calls.emplace_back("actor");
        return 0x3000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        if (address == 0x1000u + 4u &&
            target_index < dynamic_targets.size())
            return dynamic_targets[target_index++];
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address) +
                           ":" + std::to_string(value));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.emplace_back("byte:" + std::to_string(address));
        bytes[address] = value;
    }
    std::uint32_t CombatSystem() override {
        calls.emplace_back("system");
        return 0x4000u;
    }
    std::uint32_t GameDatabase() override {
        calls.emplace_back("database");
        return 0x5000u;
    }
    std::uint32_t FindMagic(std::uint32_t table,
                            std::uint32_t id) override {
        Check(table == 0x5000u + 12u && id == 0x99u,
              "magic lookup arguments");
        calls.emplace_back("magic");
        return lookup_result;
    }
    std::uint32_t FindStunt(std::uint32_t table,
                            std::uint32_t id) override {
        Check(table == 0x5000u + 12u && id == 0x99u,
              "stunt lookup arguments");
        calls.emplace_back("stunt");
        return lookup_result;
    }
    std::uint32_t FindProperty(std::uint32_t database,
                               std::uint32_t id) override {
        Check(database == 0x5000u && id == 0x99u,
              "property lookup arguments");
        calls.emplace_back("property");
        return lookup_result;
    }
    std::uint32_t ActorManager() override {
        calls.emplace_back("manager");
        return 0x7000u;
    }
    std::uint32_t CreateTask(pal4::CombatActionKind kind,
                             std::uint32_t resource,
                             std::uint32_t actor,
                             std::uint32_t target,
                             std::uint32_t auxiliary) override {
        Check(actor == 0x3000u && auxiliary == 0u,
              "task actor and auxiliary argument");
        tasks.push_back({kind, resource, actor, target, auxiliary});
        calls.emplace_back("factory");
        return next_task++;
    }
    void RegisterTask(std::uint32_t queue,
                      std::uint32_t task) override {
        Check(queue == 0x7000u + 96u && task == 0x6000u,
              "registered task and manager queue");
        calls.emplace_back("register");
    }
    void AssignOwnedChild(std::uint32_t actor,
                          std::uint32_t task) override {
        Check(actor == 0x3000u && task == 0x6001u,
              "second task ownership transfer");
        calls.emplace_back("assign");
    }
    void ResetActionComponent(std::uint32_t component) override {
        Check(component == 0x3000u + 716u,
              "actor action component offset");
        calls.emplace_back("reset");
    }
    std::uint32_t ScriptEventRouter() override {
        calls.emplace_back("router");
        return 0x8000u;
    }
    void DispatchStopSelect(std::uint32_t router,
                            std::uint32_t descriptor,
                            std::uint32_t event,
                            std::uint32_t mode) override {
        Check(router == 0x8000u && descriptor == 0x8b8b78u &&
                  event == 0x1000u && mode == 1u,
              "stop-select event arguments");
        calls.emplace_back("stop-select");
    }
};

void Prepare(Runtime& runtime, std::uint32_t kind) {
    runtime.words[0x3000u + 804u] = 1u;
    runtime.words[0x1000u + 12u] = kind;
    runtime.words[0x1000u + 8u] = 0x99u;
    runtime.words[0x1000u + 4u] = 0x11u;
}

void GatesAndMissingLookup() {
    Runtime runtime;
    runtime.words[0x3000u + 804u] = 2u;
    Check(pal4::CombatActionDispatcher::Handle(
              0x2000u, 0x1000u, runtime) == 0x3000u &&
              runtime.calls == std::vector<std::string>{
                  "actor", "read:13092"},
          "non-player actor returns pointer without changing combat state");

    for (const auto kind : {0u, 1u}) {
        Runtime missing;
        Prepare(missing, kind);
        missing.lookup_result = 0u;
        Check(pal4::CombatActionDispatcher::Handle(
                  0x2000u, 0x1000u, missing) == 0u &&
                  missing.words[0x4000u + 508u] == 2u &&
                  missing.tasks.empty() &&
                  missing.calls.back() == (kind == 0u ? "magic" : "stunt"),
              "missing skill exits before task creation and selection clear");
    }
}

void AllActionKinds() {
    for (std::uint32_t kind = 0u; kind <= 5u; ++kind) {
        Runtime runtime;
        Prepare(runtime, kind);
        if (kind == 2u) runtime.lookup_result = 0u;
        runtime.dynamic_targets = {7u, 8u};
        Check(pal4::CombatActionDispatcher::Handle(
                  0x2000u, 0x1000u, runtime) == 0u &&
                  runtime.words[0x4000u + 508u] == 2u &&
                  runtime.tasks.size() == 2u &&
                  runtime.tasks[0].kind ==
                      static_cast<pal4::CombatActionKind>(kind) &&
                  runtime.tasks[1].kind ==
                      static_cast<pal4::CombatActionKind>(kind) &&
                  runtime.tasks[0].resource ==
                      (kind == 2u ? 0u : (kind <= 1u ? 0x55u : 0u)) &&
                  runtime.bytes[0x2000u + 72u] == 0u &&
                  runtime.words[0x2000u + 64u] == 0u &&
                  runtime.words[0x2000u + 68u] == 0u,
              "all action types create two tasks and clear selection");
        const bool has_target = kind <= 2u || kind == 4u;
        Check(runtime.target_index == (has_target ? 2u : 0u) &&
                  runtime.tasks[0].target == (has_target ? 7u : 0u) &&
                  runtime.tasks[1].target == (has_target ? 8u : 0u),
              "both factories reread target only for targeted actions");
        const auto registered = std::find(runtime.calls.begin(),
                                          runtime.calls.end(), "register");
        const auto assigned = std::find(runtime.calls.begin(),
                                        runtime.calls.end(), "assign");
        const auto reset = std::find(runtime.calls.begin(),
                                     runtime.calls.end(), "reset");
        Check(registered < assigned && assigned < reset &&
                  runtime.calls[runtime.calls.size() - 4] == "stop-select" &&
                  runtime.calls[runtime.calls.size() - 3] == "byte:8264" &&
                  runtime.calls[runtime.calls.size() - 2] ==
                      "write:8256:0" &&
                  runtime.calls.back() == "write:8260:0",
              "queue registration precedes ownership and ordered tail clear");
    }

    Runtime unknown;
    Prepare(unknown, 9u);
    Check(pal4::CombatActionDispatcher::Handle(
              0x2000u, 0x1000u, unknown) == 0u &&
              unknown.tasks.empty() &&
              std::find(unknown.calls.begin(), unknown.calls.end(),
                        "reset") != unknown.calls.end() &&
              unknown.calls.back() == "write:8260:0",
          "unknown action still performs common selection cleanup");
}

}  // namespace

int main() {
    try {
        GatesAndMissingLookup();
        AllActionKinds();
        std::cout << "combat action dispatcher tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
