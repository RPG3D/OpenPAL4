#include "pal4/actor_manager_player_maintenance.h"

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

constexpr std::uint32_t actor = 0x1000u;

struct Runtime final : pal4::IActorManagerPlayerMaintenanceRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;
    std::vector<std::uint32_t> inventory;
    std::vector<std::uint32_t> indexed;
    std::vector<std::uint32_t> sorted;
    bool task_result = false;
    bool extend_during_fade = false;
    bool throw_on_sorted = false;
    std::uint32_t fade_count = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == actor + 1288u, "AI task slot offset");
        trace.emplace_back("read_task");
        return words[address];
    }
    bool ProcessAIActionTask(std::uint32_t task) override {
        Check(task == words[actor + 1288u], "task pointer forwarded");
        trace.emplace_back("task");
        return task_result;
    }
    void CleanupObjectLists(std::uint32_t actual_actor) override {
        Check(actual_actor == actor, "object-list cleanup actor");
        trace.emplace_back("cleanup");
    }
    void ProcessInventory(std::uint32_t slots,
                          std::vector<std::uint32_t>& items) override {
        Check(slots == actor + 896u && items.empty() &&
                  items.capacity() >= 32u,
              "inventory sees empty 32-pointer collector");
        trace.emplace_back("inventory");
        items.insert(items.end(), inventory.begin(), inventory.end());
    }
    void ProcessIndexedCollection(
        std::uint32_t collection,
        std::vector<std::uint32_t>& items) override {
        Check(collection == actor + 920u &&
                  items.size() == inventory.size(),
              "indexed collection follows inventory");
        trace.emplace_back("indexed");
        items.insert(items.end(), indexed.begin(), indexed.end());
    }
    void ProcessSortedValues(std::uint32_t collection,
                             std::vector<std::uint32_t>& items) override {
        Check(collection == actor + 920u &&
                  items.size() == inventory.size() + indexed.size(),
              "sorted collection follows indexed values");
        trace.emplace_back("sorted");
        if (throw_on_sorted) throw std::runtime_error("sorted processing failed");
        items.insert(items.end(), sorted.begin(), sorted.end());
    }
    void HandleGameObjectFade(std::uint32_t receiver,
                              std::uint32_t item,
                              std::uint32_t actual_actor,
                              std::vector<std::uint32_t>& items) override {
        Check(receiver == actor && actual_actor == actor,
              "fade receiver and owner both use actor");
        trace.emplace_back("fade:" + std::to_string(item));
        ++fade_count;
        if (extend_during_fade && fade_count == 1u)
            items.push_back(0x4000u);
    }
};

void TaskForwarding() {
    Runtime runtime;
    runtime.words[actor + 1288u] = 0x9000u;
    runtime.task_result = true;
    Check(pal4::ActorManagerPlayerMaintenance::ProcessTask(actor, runtime) &&
              runtime.trace ==
                  std::vector<std::string>{"read_task", "task"},
          "AI task result is forwarded after reading actor slot");
    runtime.trace.clear();
    runtime.words[actor + 1288u] = 0u;
    runtime.task_result = false;
    Check(!pal4::ActorManagerPlayerMaintenance::ProcessTask(actor, runtime) &&
              runtime.trace ==
                  std::vector<std::string>{"read_task", "task"},
          "null task is still passed to native task processor");
}

void EmptyCleanup() {
    Runtime runtime;
    Check(pal4::ActorManagerPlayerMaintenance::CleanupData(actor, runtime) ==
              0u &&
              runtime.trace == std::vector<std::string>{
                  "cleanup", "inventory", "indexed", "sorted"} &&
              runtime.fade_count == 0u,
          "empty collector preserves cleanup and three source calls");
}

void OrderedAndExtendedCleanup() {
    Runtime runtime;
    runtime.inventory = {0x2000u, 0x2100u};
    runtime.indexed = {0x3000u};
    runtime.sorted = {0x3100u};
    runtime.extend_during_fade = true;
    Check(pal4::ActorManagerPlayerMaintenance::CleanupData(actor, runtime) ==
              0u &&
              runtime.trace == std::vector<std::string>{
                  "cleanup", "inventory", "indexed", "sorted",
                  "fade:8192", "fade:8448", "fade:12288", "fade:12544",
                  "fade:16384"} &&
              runtime.fade_count == 5u,
          "collector preserves source order and rereads end after fade callbacks");
}

void ProcessingFailure() {
    Runtime runtime;
    runtime.inventory = {0x2000u};
    runtime.throw_on_sorted = true;
    bool caught = false;
    try {
        static_cast<void>(
            pal4::ActorManagerPlayerMaintenance::CleanupData(actor, runtime));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.trace == std::vector<std::string>{
              "cleanup", "inventory", "indexed", "sorted"} &&
              runtime.fade_count == 0u,
          "source exception unwinds collector before fade dispatch");
}

}  // namespace

int main() {
    try {
        TaskForwarding();
        EmptyCleanup();
        OrderedAndExtendedCleanup();
        ProcessingFailure();
        std::cout << "actor_manager_player_maintenance_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
