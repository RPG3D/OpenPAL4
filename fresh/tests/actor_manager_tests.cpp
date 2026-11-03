#include "pal4/actor_manager.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IActorManagerLifecycleRuntime,
                       pal4::IActorManagerUpdateRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> trace;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> destroyed;
    std::vector<std::uint32_t> visited;
    std::vector<std::uint32_t> components;
    std::vector<std::uint32_t> renderables;
    std::uint32_t next = 0x8000u;
    std::uint32_t fail_allocation = 0;
    std::uint32_t allocations = 0;
    std::uint32_t registration_result = 0x5000u;
    std::uint32_t task_result = 0x1234u;
    std::uint32_t dynamic_end = 0;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t Allocate(const std::uint32_t bytes_count) override {
        trace.push_back("allocate:" + std::to_string(bytes_count));
        if (++allocations == fail_allocation) return 0;
        const auto result = next;
        next += 0x100u;
        return result;
    }
    std::uint32_t Free(const std::uint32_t address) override {
        trace.push_back("free:" + std::to_string(address));
        freed.push_back(address);
        return address ? address + 1u : 0u;
    }
    std::uint32_t SingletonManager() override {
        trace.push_back("singleton-manager");
        return 0x4444u;
    }
    std::uint32_t RegisterSingleton(
        const std::uint32_t manager, const std::string_view name,
        const std::uint32_t id, const std::uint32_t factory,
        const std::uint32_t destructor) override {
        Check(manager == 0x4444u && name == "cbActorManager" &&
              id == 1029u && factory == 0x5297b0u &&
              destructor == 0x529810u, "actor singleton registration fields");
        trace.push_back("register");
        return registration_result;
    }
    void InitializePrimary(const std::uint32_t block) override {
        trace.push_back("init-primary");
        Check(block != 0u, "primary block address");
    }
    void InitializeSecondary(const std::uint32_t block) override {
        trace.push_back("init-secondary");
        Check(block != 0u, "secondary block address");
    }
    void InitializeTertiary(const std::uint32_t block) override {
        trace.push_back("init-tertiary");
        for (const auto offset : {4u, 8u, 12u, 20u, 24u, 28u})
            words[block + offset] = 0;
    }
    std::uint32_t CleanupPrimary(const std::uint32_t) override {
        trace.push_back("cleanup-primary");
        return 0xfaceu;
    }
    void CleanupSecondary(const std::uint32_t) override {
        trace.push_back("cleanup-secondary");
    }
    void EraseTaskNode(const std::uint32_t block,
                       const std::uint32_t node) override {
        Check(block != 0u, "task block address");
        trace.push_back("erase:" + std::to_string(node));
    }
    void DestroyOwned(const std::uint32_t object,
                      const std::uint32_t flags) override {
        Check(flags == 1u, "owned object deleting destructor flag");
        destroyed.push_back(object);
        trace.push_back("destroy:" + std::to_string(object));
    }
    void ProcessEvent41(const std::uint32_t collection) override {
        trace.push_back("event41:" + std::to_string(collection));
    }
    void CleanupData5(const std::uint32_t manager) override {
        trace.push_back("cleanup-data5:" + std::to_string(manager));
    }
    std::uint32_t ResetTaskFlags(const std::uint32_t queue) override {
        trace.push_back("reset-flags:" + std::to_string(queue));
        return 0xabcdu;
    }
    void ProcessEvent40(const std::uint32_t collection,
                        const std::uint32_t argument) override {
        Check(argument == 0x3f800000u, "update argument bits");
        trace.push_back("event:" + std::to_string(collection));
    }
    std::uint32_t UpdateTaskQueue(const std::uint32_t queue,
                                  const std::uint32_t argument) override {
        Check(argument == 0x3f800000u, "task argument bits");
        trace.push_back("task:" + std::to_string(queue));
        return task_result;
    }
    void SetActorProperty(const std::uint32_t actor,
                          const std::uint32_t) override {
        visited.push_back(actor);
        if (dynamic_end) {
            words[0x3000u + 40u] = dynamic_end;
            dynamic_end = 0;
        }
    }
    void UpdateComponent(const std::uint32_t component,
                         const std::uint32_t) override {
        components.push_back(component);
    }
    void UpdateRenderableCollection(const std::uint32_t collection,
                                    const std::uint32_t) override {
        renderables.push_back(collection);
    }
};

void SingletonAndConstruction() {
    Runtime runtime;
    pal4::ActorManagerSingletonState state{0x80u, 0};
    Check(pal4::ActorManagerSingleton::Get(state, runtime) == 0x5000u &&
          state.guard == 0x81u && state.instance == 0x5000u &&
          runtime.trace ==
              std::vector<std::string>{"singleton-manager", "register"},
          "singleton registers once after setting shared guard");
    runtime.trace.clear();
    Check(pal4::ActorManagerSingleton::Get(state, runtime) == 0x5000u &&
          runtime.trace.empty(), "singleton reuses registered instance");
    const auto manager = pal4::ActorManagerSingleton::Create(runtime);
    const auto head = runtime.words[manager + 100u];
    Check(manager == 0x8000u && head == 0x8100u &&
          runtime.bytes[manager + 96u] == 0u &&
          runtime.words[head] == head &&
          runtime.words[head + 4u] == head &&
          runtime.words[manager + 104u] == 0u &&
          runtime.trace == std::vector<std::string>{
              "allocate:108", "init-primary", "init-secondary",
              "init-tertiary", "allocate:16"},
          "108-byte actor manager initializes children then task list");
}

void ConstructionFailures() {
    Runtime runtime;
    runtime.fail_allocation = 1;
    Check(pal4::ActorManagerSingleton::Create(runtime) == 0u &&
          runtime.trace == std::vector<std::string>{"allocate:108"},
          "manager allocation failure skips child initialization");
    Runtime sentinel_failure;
    sentinel_failure.fail_allocation = 2;
    Check(pal4::ActorManagerSingleton::Create(sentinel_failure) == 0u &&
          sentinel_failure.freed.back() == 0x8000u &&
          sentinel_failure.trace[sentinel_failure.trace.size() - 3] ==
              "cleanup-secondary" &&
          sentinel_failure.trace[sentinel_failure.trace.size() - 2] ==
              "cleanup-primary",
          "task sentinel failure unwinds children and frees manager");
}

void OwnedArraysAndDestruction() {
    Runtime runtime;
    const auto manager = pal4::ActorManagerSingleton::Create(runtime);
    const auto head = runtime.words[manager + 100u];
    constexpr auto first_task = 0xa000u;
    constexpr auto second_task = 0xa100u;
    runtime.words[head] = first_task;
    runtime.words[first_task] = second_task;
    runtime.words[second_task] = head;
    runtime.words[manager + 104u] = 2u;
    const auto block = manager + 64u;
    runtime.words[block + 20u] = 0xb000u;
    runtime.words[block + 24u] = 0xb010u;
    runtime.words[block + 28u] = 0xb010u;
    runtime.bytes[0xb000u] = 1;
    runtime.words[0xb004u] = 0xc000u;
    runtime.bytes[0xb008u] = 0;
    runtime.words[0xb00cu] = 0xc100u;
    runtime.words[block + 4u] = 0xd000u;
    runtime.words[block + 8u] = 0xd008u;
    runtime.words[block + 12u] = 0xd008u;
    runtime.bytes[0xd000u] = 1;
    runtime.words[0xd004u] = 0xe000u;
    runtime.trace.clear();
    Check(pal4::ActorManagerSingleton::Destroy(manager, runtime) ==
              manager + 1u &&
          runtime.destroyed == std::vector<std::uint32_t>{0xc000u, 0xe000u} &&
          runtime.words[manager + 100u] == 0u &&
          runtime.words[manager + 104u] == 0u &&
          runtime.words[block + 20u] == 0u &&
          runtime.words[block + 24u] == 0u &&
          runtime.words[block + 28u] == 0u &&
          runtime.words[block + 4u] == 0u &&
          runtime.words[block + 8u] == 0u &&
          runtime.words[block + 12u] == 0u,
          "destruction releases owned entries and clears all array fields");
    Check(runtime.trace == std::vector<std::string>{
              "erase:40960", "erase:41216", "free:33024",
              "destroy:49152", "free:45056", "destroy:57344",
              "free:53248", "cleanup-secondary", "cleanup-primary",
              "free:32768"},
          "task list, tertiary arrays, secondary and primary release order");
    Check(pal4::ActorManagerSingleton::Destroy(0, runtime) == 0,
          "null manager destruction has defined no-op result");
}

void UpdatePaths() {
    Runtime runtime;
    constexpr auto manager = 0x3000u;
    constexpr auto argument = 0x3f800000u;
    Check(pal4::ActorManagerUpdate::UpdateData(
              manager, argument, runtime) == runtime.task_result &&
          runtime.trace == std::vector<std::string>{
              "event:12320", "event:12288", "task:12384"},
          "update processes +32, base and +96 queue in order");
    runtime.words[manager + 36u] = 0x4000u;
    runtime.words[manager + 40u] = 0x4004u;
    runtime.words[0x4000u] = 0x5000u;
    runtime.words[0x4004u] = 0x5100u;
    runtime.words[manager + 4u] = 0x6000u;
    runtime.words[manager + 8u] = 0x6004u;
    runtime.words[0x6000u] = 0x5200u;
    runtime.dynamic_end = 0x4008u;
    Check(pal4::ActorManagerUpdate::ProcessActors(
              manager, argument, runtime) == 0x6004u &&
          runtime.visited ==
              std::vector<std::uint32_t>{0x5000u, 0x5100u, 0x5200u},
          "actor update rereads first collection end after callback");
    runtime.words[0x5000u + 808u] = 0x7000u;
    runtime.words[0x5100u + 808u] = 0u;
    runtime.words[0x5200u + 808u] = 0x7200u;
    Check(pal4::ActorManagerUpdate::UpdateComponents(
              manager, argument, runtime) == 0x6004u &&
          runtime.components ==
              std::vector<std::uint32_t>{0x7000u, 0x7200u},
          "component update skips null pointers at actor +808");
    Check(pal4::ActorManagerUpdate::UpdateRenderables(
              manager, argument, runtime) == 0x6004u &&
          runtime.renderables ==
              std::vector<std::uint32_t>{0x5000u + 728u,
                                              0x5100u + 728u,
                                              0x5200u + 728u},
          "renderable update passes each actor +728 collection");
}

void AuxiliaryCleanup() {
    Runtime runtime;
    constexpr auto manager = 0x3000u;
    Check(pal4::ActorManagerLifecycle::DestroyData(manager, runtime) ==
              0xabcdu &&
          runtime.trace == std::vector<std::string>{
              "event41:12320", "cleanup-data5:12288",
              "reset-flags:12384"},
          "actor manager data destructor keeps collection and flag order");
    constexpr auto block = 0x4000u;
    runtime.words[block + 4u] = 0x5000u;
    runtime.words[block + 8u] = 0x5010u;
    runtime.words[block + 12u] = 0x5010u;
    runtime.bytes[0x5000u] = 1;
    runtime.words[0x5004u] = 0x6000u;
    runtime.bytes[0x5008u] = 1;
    runtime.words[0x500cu] = 0;
    Check(pal4::ActorManagerLifecycle::CleanupOwnedList(block, runtime) ==
              0x5001u &&
          runtime.destroyed == std::vector<std::uint32_t>{0x6000u} &&
          runtime.freed.back() == 0x5000u &&
          runtime.words[block + 4u] == 0u &&
          runtime.words[block + 8u] == 0u &&
          runtime.words[block + 12u] == 0u,
          "single owned list destroys non-null pointers and returns free result");
}

}  // namespace

int main() {
    try {
        SingletonAndConstruction();
        ConstructionFailures();
        OwnedArraysAndDestruction();
        UpdatePaths();
        AuxiliaryCleanup();
        std::cout << "actor_manager_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
