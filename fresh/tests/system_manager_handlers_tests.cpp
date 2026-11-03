#include "pal4/system_manager_handlers.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Registration {
    std::uint32_t mode;
    std::uint32_t key;
    std::uint32_t owner;
    std::uint32_t callback;
};

struct Runtime final : pal4::ISystemManagerLifecycleRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> groups;
    std::vector<Registration> registrations;
    std::vector<std::uint32_t> cleaned;
    std::vector<std::uint32_t> freed;
    std::uint32_t router_calls = 0;
    std::uint32_t group_calls = 0;
    std::uint32_t skill_calls = 0;
    std::uint32_t next = 0x10000u;
    std::uint32_t ReadWord(const std::uint32_t address) override {
        if (address == 0x2004u) ++group_calls;
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
    }
    void TouchSkillSystem() override { ++skill_calls; }
    std::uint32_t Allocate(const std::uint32_t size) override {
        Check(size == 12u || size == 20u || size == 32u,
              "router allocation size");
        const auto address = next;
        next += 0x100u;
        return address;
    }
    std::uint32_t EventRouter() override {
        ++router_calls;
        return 0x2000u;
    }
    std::uint32_t Register(const std::uint32_t router,
                           const std::uint32_t mode,
                           const std::uint32_t key,
                           const std::uint32_t owner,
                           const std::uint32_t callback) override {
        Check(router == 0x2000u, "router passed to registration");
        registrations.push_back({mode, key, owner, callback});
        return 0x3000u + static_cast<std::uint32_t>(registrations.size());
    }
    void CleanupPayload(const std::uint32_t address) override {
        cleaned.push_back(address);
    }
    void Free(const std::uint32_t address) override {
        freed.push_back(address);
    }
    std::uint32_t DeleteCallback(const std::uint32_t address,
                                 const std::uint32_t flags) override {
        Check(flags == 1u, "owned callback delete flag");
        freed.push_back(address);
        return address;
    }
};

constexpr auto manager = 0x1000u;
constexpr std::array<std::uint32_t, 13> globals{
    0x8b62e0u, 0x8b5f8cu, 0x8b599cu, 0x8b5998u, 0x8b5f90u,
    0x8b62e4u, 0x8b8b8cu, 0x8b8b84u, 0x8b8b88u, 0x8b8b90u,
    0x8b5950u, 0x8b5958u, 0x8b5954u};
constexpr std::array<std::uint32_t, 13> modes{
    2u, 2u, 2u, 2u, 2u, 2u, 2u, 3u, 3u, 2u, 3u, 3u, 3u};
constexpr std::array<std::uint32_t, 13> callbacks{
    0x527ff0u, 0x527ff0u, 0x527ff0u, 0x528080u, 0x528080u,
    0x528080u, 0x5280c0u, 0x5280d0u, 0x5283d0u, 0x5283e0u,
    0x5286d0u, 0x528730u, 0x528700u};

void InitializeGroups(Runtime& runtime) {
    constexpr auto router = 0x2000u;
    constexpr auto tree_head = 0x4000u;
    constexpr auto nil = 0x8f609cu;
    runtime.words[router + 4u] = tree_head;
    runtime.words[tree_head] = tree_head;
    runtime.words[tree_head + 4u] = nil;
    runtime.words[tree_head + 8u] = tree_head;
    runtime.words[nil + 28u] = 1;
    for (auto index = 0u; index < globals.size(); ++index) {
        const auto key = 0x5000u + index;
        runtime.words[globals[index]] = key;
        const auto group = pal4::SystemEventRouterTree::FindOrCreateGroup(
            router, key, nil, runtime);
        runtime.groups[key] = group;
    }
    runtime.group_calls = 0;
}

void DefaultRegistrationBridge() {
    Runtime runtime;
    InitializeGroups(runtime);
    const auto handle = runtime.pal4::ISystemManagerHandlerRuntime::Register(
        0x2000u, 3u, 0x5000u, manager, 0x5280d0u);
    const auto group = runtime.groups.at(0x5000u);
    const auto head = runtime.words[group + 4u];
    const auto node = runtime.words[head];
    Check(handle != 0 && runtime.words[handle] == 0x844910u &&
          runtime.words[handle + 4u] == manager &&
          runtime.words[handle + 8u] == 0x5280d0u &&
          runtime.words[node + 12u] == handle &&
          runtime.words[node + 16u] == 3u &&
          runtime.words[group + 8u] == 1u,
          "handler runtime default registration uses the real event tree");
}

void Attach(Runtime& runtime, const std::uint32_t index,
            const std::uint32_t handle) {
    const auto group = runtime.groups.at(0x5000u + index);
    const auto head = runtime.words[group + 4u];
    const auto node = 0xa000u + index * 0x100u;
    runtime.words[head] = node;
    runtime.words[head + 4u] = node;
    runtime.words[node] = head;
    runtime.words[node + 4u] = head;
    runtime.words[node + 12u] = handle;
    runtime.words[group + 8u] = 1;
}

void RegisterAndReplace() {
    Runtime runtime;
    InitializeGroups(runtime);
    pal4::SystemManagerHandlers::Initialize(manager, runtime);
    Check(runtime.registrations.size() == 13 && runtime.router_calls == 13 &&
          runtime.group_calls == 0, "initial registration count");
    for (auto index = 0u; index < globals.size(); ++index) {
        const auto& registration = runtime.registrations[index];
        Check(registration.mode == modes[index] &&
              registration.key == 0x5000u + index &&
              registration.owner == manager &&
              registration.callback == callbacks[index] &&
              runtime.words[manager + 4u + index * 8u] == 0x3001u + index &&
              runtime.words[manager + 8u + index * 8u] == 0x5000u + index,
              "ordered handler registration and pair layout");
    }
    Attach(runtime, 0, 0x3001u);
    Attach(runtime, 1, 0xdeadu);
    Attach(runtime, 12, 0x300du);
    pal4::SystemManagerHandlers::Initialize(manager, runtime);
    Check(runtime.registrations.size() == 26 && runtime.router_calls == 39 &&
          runtime.group_calls == 26 && runtime.words[0x200cu] == 13,
          "replacement searches existing tree groups before registration");
    Check(runtime.cleaned == std::vector<std::uint32_t>({0xa008u, 0xac08u}) &&
          runtime.freed == std::vector<std::uint32_t>({0xa000u, 0xac00u}),
          "matched list nodes release in slot order");
    for (const auto index : {0u, 12u}) {
        const auto group = runtime.groups.at(0x5000u + index);
        const auto head = runtime.words[group + 4u];
        Check(runtime.words[head] == head &&
              runtime.words[head + 4u] == head &&
              runtime.words[group + 8u] == 0,
              "matched list unlinked and count decremented");
    }
    Check(runtime.words[0xa100u + 12u] == 0xdeadu &&
          runtime.words[runtime.words[runtime.groups.at(0x5001u) + 4u]] ==
              0xa100u,
          "unmatched list entry remains linked");
    Check(runtime.words[manager + 4u] == 0x300eu &&
          runtime.words[manager + 4u + 12u * 8u] == 0x301au,
          "replacement handles overwrite old pairs");
}

void EmptyCleanup() {
    Runtime runtime;
    Check(pal4::SystemManagerHandlers::Cleanup(manager, runtime) == 0 &&
          runtime.router_calls == 0 && runtime.group_calls == 0,
          "empty cleanup avoids router access");
}

void Lifecycle() {
    Runtime runtime;
    InitializeGroups(runtime);
    runtime.words[manager + 112u] = 0xdeadu;
    runtime.words[manager + 116u] = 0xbeefu;
    runtime.bytes[manager + 108u] = 0xffu;
    runtime.bytes[manager + 124u] = 0xffu;
    Check(pal4::SystemManagerLifecycle::Construct(manager, runtime) == manager &&
          runtime.words[manager] == 0x844908u &&
          runtime.words[manager + 112u] == 0 &&
          runtime.words[manager + 116u] == 0 &&
          runtime.words[manager + 120u] == 0 &&
          runtime.bytes[manager + 108u] == 0 &&
          runtime.bytes[manager + 124u] == 0 &&
          runtime.skill_calls == 1 && runtime.registrations.size() == 13,
          "constructor initializes 128-byte manager and subscribes");
    runtime.words[manager + 112u] = 0x7000u;
    runtime.words[manager + 116u] = 0x7010u;
    runtime.words[manager + 120u] = 0x7080u;
    Check(pal4::SystemManagerLifecycle::UpdateState(manager, runtime) ==
              0x7010u &&
          runtime.words[manager + 116u] == 0x7000u &&
          runtime.words[manager + 120u] == 0x7080u &&
          runtime.skill_calls == 2,
          "update returns prior end and resets it to begin");
    Check(pal4::SystemManagerLifecycle::Destruct(manager, runtime) == 0 &&
          runtime.freed == std::vector<std::uint32_t>{0x7000u} &&
          runtime.words[manager + 112u] == 0 &&
          runtime.words[manager + 116u] == 0 &&
          runtime.words[manager + 120u] == 0 &&
          runtime.words[manager + 4u] != 0,
          "destructor frees buffer without clearing event slots");
}

void LifecycleHelpers() {
    Runtime runtime;
    runtime.bytes[manager + 124u] = 1u;
    pal4::SystemManagerLifecycle::Deactivate(manager, runtime);
    Check(runtime.bytes[manager + 124u] == 0 && runtime.skill_calls == 1,
          "deactivate touches SkillSystem and clears active byte");
    pal4::SystemManagerLifecycle::Activate(manager, runtime);
    Check(runtime.bytes[manager + 124u] == 1 && runtime.skill_calls == 2,
          "slot0 touches SkillSystem and pauses the action sequence");
    constexpr auto block = 0x9000u;
    runtime.words[block + 4u] = 0xa000u;
    runtime.words[block + 8u] = 0xa020u;
    runtime.words[block + 12u] = 0xa100u;
    pal4::SystemManagerLifecycle::CleanupMemoryBlock(block, runtime);
    Check(runtime.freed == std::vector<std::uint32_t>{0xa000u} &&
          runtime.words[block + 4u] == 0 &&
          runtime.words[block + 8u] == 0 &&
          runtime.words[block + 12u] == 0,
          "embedded buffer cleanup releases begin and clears three pointers");
}

struct CallbackRuntime final : pal4::ISystemManagerCallbackRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint32_t> internal;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> appended;
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t InternalObject(const std::uint32_t event_object) override {
        calls.emplace_back("internal");
        return internal[event_object];
    }
    void PrepareObject(const std::uint32_t object) override {
        Check(object == 0x5000u, "prepared internal object");
        calls.emplace_back("prepare");
    }
    void AppendObject(const std::uint32_t owner,
                      const std::uint32_t object) override {
        Check(owner == manager, "append owner");
        calls.emplace_back("append");
        appended.push_back(object);
    }
    void TouchSkillSystem() override { calls.emplace_back("skill"); }
    void Update(const std::uint32_t owner) override {
        Check(owner == manager, "update owner");
        calls.emplace_back("update");
    }
    std::uint32_t InvokeManagerSlot0(const std::uint32_t owner) override {
        Check(owner == manager, "virtual owner");
        calls.emplace_back("virtual-zero");
        return 0xdecau;
    }
};

void CallbackCases() {
    CallbackRuntime runtime;
    runtime.internal[0x3000u] = 0x5000u;
    runtime.words[manager + 112u] = 0x2000u;
    runtime.words[manager + 116u] = 0x2000u;
    pal4::SystemManagerCallbacks::AddObject(manager, 0x3000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "internal", "prepare", "append", "skill", "update"} &&
          runtime.appended == std::vector<std::uint32_t>{0x5000u},
          "new internal object is prepared, appended and updated");
    runtime.calls.clear();
    runtime.words[manager + 116u] = 0x2008u;
    runtime.words[0x2000u] = 0x6000u;
    runtime.words[0x2004u] = 0x5000u;
    pal4::SystemManagerCallbacks::AddObject(manager, 0x3000u, runtime);
    Check(runtime.calls == std::vector<std::string>{"internal", "prepare"} &&
          runtime.appended.size() == 1,
          "duplicate object still prepares but does not append or update");
    runtime.calls.clear();
    pal4::SystemManagerCallbacks::AddObject(manager, 0x4000u, runtime);
    Check(runtime.calls == std::vector<std::string>{"internal", "skill"},
          "null internal object only touches skill system");
    runtime.calls.clear();
    pal4::SystemManagerCallbacks::ProcessObject(manager, 0x3000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "internal", "skill", "update"} &&
          runtime.words[manager + 116u] == 0x2004u &&
          runtime.words[0x2000u] == 0x6000u,
          "process callback removes before update");
    runtime.calls.clear();
    Check(pal4::SystemManagerCallbacks::InvokeManager(manager, runtime) ==
              0xdecau &&
          runtime.calls == std::vector<std::string>{"virtual-zero"},
          "virtual callback forwards slot zero return");
}

}  // namespace

int main() {
    try {
        EmptyCleanup();
        DefaultRegistrationBridge();
        RegisterAndReplace();
        Lifecycle();
        LifecycleHelpers();
        CallbackCases();
        std::cout << "system_manager_handlers_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
