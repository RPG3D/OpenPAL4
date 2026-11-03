#include "pal4/combat_role_state_registration.h"
#include "pal4/system_event_router_tree.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr auto kWindow = 0x4000u;
constexpr auto kTree = 0x5000u;
constexpr auto kHead = 0x6000u;
constexpr auto kNil = 0x7000u;
constexpr std::array<std::uint32_t, 9> kKeyGlobals{{
    0x8b62e0u, 0x8b6310u, 0x8b62e4u, 0x8b62f0u, 0x8b8b78u,
    0x8b5958u, 0x8b599cu, 0x8b62f0u, 0x8b8b84u,
}};
constexpr std::array<std::uint32_t, 9> kCallbacks{{
    0x5518a0u, 0x551be0u, 0x5518c0u, 0x5518e0u, 0x551ab0u,
    0x5519d0u, 0x551a70u, 0x551a70u, 0x551ae0u,
}};

struct SubscriptionRuntime final : pal4::IEventSubscriptionRuntime,
                                   pal4::ISystemEventRouterTreeRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> cleaned;
    std::vector<std::uint32_t> freed;
    std::uint32_t next = 0x10000u;
    std::uint32_t registrations = 0u;
    std::uint32_t router_calls = 0u;

    SubscriptionRuntime() {
        words[kTree + 4u] = kHead;
        words[kHead] = kHead;
        words[kHead + 4u] = kNil;
        words[kHead + 8u] = kHead;
        words[kNil + 28u] = 1u;
        for (std::uint32_t index = 0u; index < kKeyGlobals.size(); ++index)
            words[kKeyGlobals[index]] = 0x200u + index;
        words[0x8b62f0u] = 0x203u;
    }
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
    std::uint32_t Allocate(const std::uint32_t) override {
        const auto value = next;
        next += 0x100u;
        return value;
    }
    void Free(const std::uint32_t value) override { freed.push_back(value); }
    void CleanupPayload(const std::uint32_t value) override {
        cleaned.push_back(value);
    }
    std::uint32_t DeleteCallback(const std::uint32_t,
                                 const std::uint32_t) override {
        return 0u;
    }
    std::uint32_t EventKey(const std::string_view) override {
        throw std::runtime_error("role state uses global event keys");
    }
    std::uint32_t EventRouter() override {
        ++router_calls;
        return kTree;
    }
    std::uint32_t Register(const std::uint32_t router,
                           const std::uint32_t mode,
                           const std::uint32_t key,
                           const std::uint32_t owner,
                           const std::uint32_t callback) override {
        const auto index = registrations % 9u;
        ++registrations;
        Check(router == kTree && mode == 2u && owner == kWindow &&
              key == words[kKeyGlobals[index]] &&
              callback == kCallbacks[index],
              "role-state subscriptions retain IDA key and callback order");
        return pal4::SystemEventRouterTree::Register(
            router, mode, key, owner, callback, kNil, *this);
    }
    std::uint32_t FindEventGroup(const std::uint32_t router,
                                 const std::uint32_t key) override {
        return pal4::SystemEventRouterTree::FindOrCreateGroup(
            router, key, kNil, *this);
    }
    void ReleaseOwnedSlot(const std::uint32_t slot) override {
        CleanupPayload(slot);
    }
    void FreeNode(const std::uint32_t node) override { Free(node); }
};

struct FactoryRuntime final : pal4::ICombatRoleStateFactoryRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t next = 0x9000u;
    std::uint32_t event_tree = 0u;
    std::uint32_t event_key = 0u;
    std::uint32_t event_callback = 0u;
    std::uint32_t deleted_callback = 0u;
    std::uint32_t delete_flags = 0u;
    bool fail_allocation = false;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read");
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        calls.emplace_back(bytes == 0x20cu ? "allocate524" : "allocate8");
        if (fail_allocation) return 0u;
        const auto value = next;
        next += 0x100u;
        return value;
    }
    std::uint32_t ConstructRoleState(
        const std::uint32_t storage,
        const std::uint32_t layout_name) override {
        calls.emplace_back("construct");
        Check(layout_name == 0x1234u,
              "role-state factory forwards the layout argument");
        return storage + 4u;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x7000u;
    }
    std::uint32_t InsertFactory(
        const std::uint32_t tree,
        const std::uint32_t callback) override {
        calls.emplace_back("factory");
        Check(tree == 0x8b694cu &&
              callback == pal4::CombatRoleStateFactory::kCreateCallback,
              "role-state factory uses cached type and create entry");
        return 0xabcu;
    }
    void InsertEvent(const std::uint32_t tree,
                     const std::uint32_t key,
                     std::uint32_t& callback) override {
        calls.emplace_back("event");
        event_tree = tree;
        event_key = key;
        event_callback = callback;
    }
    void DeleteCallback(const std::uint32_t callback,
                        const std::uint32_t flags) override {
        calls.emplace_back("delete");
        deleted_callback = callback;
        delete_flags = flags;
    }
};

void NineSubscriptions() {
    SubscriptionRuntime runtime;
    const auto last = pal4::CombatRoleStateSubscriptions::Subscribe(
        kWindow, runtime);
    Check(last && runtime.registrations == 9u &&
          runtime.router_calls == 9u &&
          runtime.words[kTree + 12u] == 8u,
          "nine subscriptions create eight distinct key groups");
    for (std::uint32_t index = 0u; index < 9u; ++index) {
        const auto handle = runtime.words[kWindow + 40u + 8u * index];
        const auto key = runtime.words[kWindow + 44u + 8u * index];
        Check(handle && key == runtime.words[kKeyGlobals[index]] &&
              runtime.words[handle + 4u] == kWindow &&
              runtime.words[handle + 8u] == kCallbacks[index],
              "subscription fields and callback payload match every binding");
    }
    const auto duplicate_group = pal4::SystemEventRouterTree::FindExact(
        kTree, 0x203u, kNil, runtime) + 16u;
    Check(runtime.words[duplicate_group + 8u] == 2u,
          "two role-state callbacks share the repeated key group");

    const auto replacement = pal4::CombatRoleStateSubscriptions::Subscribe(
        kWindow, runtime);
    Check(replacement && replacement != last &&
          runtime.registrations == 18u &&
          runtime.cleaned.size() == 9u && runtime.freed.size() == 9u &&
          runtime.words[kTree + 12u] == 8u &&
          runtime.words[duplicate_group + 8u] == 2u,
          "repeat subscription releases all old nodes before replacing them");
    Check(pal4::CombatRoleStateSubscriptions::Clear(kWindow, runtime) == 0u &&
          runtime.cleaned.size() == 18u && runtime.freed.size() == 18u &&
          runtime.words[duplicate_group + 8u] == 0u,
          "clear releases every owned callback including duplicate-key nodes");
    for (std::uint32_t index = 0u; index < 9u; ++index)
        Check(runtime.words[kWindow + 40u + 8u * index] == 0u &&
              runtime.words[kWindow + 44u + 8u * index] == 0u,
              "clear zeros all handle/key pairs");
    const auto calls = runtime.router_calls;
    Check(pal4::CombatRoleStateSubscriptions::Clear(kWindow, runtime) == 0u &&
          runtime.router_calls == calls,
          "clear of empty subscriptions skips router lookup");
}

void FactoryLifecycle() {
    FactoryRuntime runtime;
    runtime.words[pal4::CombatRoleStateFactory::kFactorySource] = 0x8b694cu;
    Check(pal4::CombatRoleStateFactory::GetInstanceThunk(runtime) ==
              0x8b694cu &&
          runtime.words[pal4::CombatRoleStateFactory::kFactoryCache] ==
              0x8b694cu &&
          runtime.calls == std::vector<std::string>{"read", "write"},
          "role-state getter caches the pointer value without extra dereference");
    runtime.calls.clear();
    Check(pal4::CombatRoleStateFactory::GetInstance(runtime) == 0x8b694cu &&
          runtime.calls == std::vector<std::string>{"read", "write"},
          "direct getter matches its thunk");
    runtime.calls.clear();
    Check(pal4::CombatRoleStateFactory::RegisterFactoryThunk(runtime) ==
              0xabcu &&
          runtime.calls ==
              std::vector<std::string>{"manager", "read", "factory"},
          "factory registration reads the cache after manager lookup");
    runtime.calls.clear();
    runtime.fail_allocation = true;
    Check(pal4::CombatRoleStateFactory::Create(0x1234u, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{"allocate524"},
          "failed 524-byte allocation skips construction");
    runtime.calls.clear();
    runtime.fail_allocation = false;
    Check(pal4::CombatRoleStateFactory::Create(0x1234u, runtime) == 0x9004u &&
          runtime.calls ==
              std::vector<std::string>{"allocate524", "construct"},
          "factory forwards the layout argument to the constructor");
    runtime.calls.clear();
    Check(pal4::CombatRoleStateFactory::RegisterEventThunk(runtime) ==
              pal4::CombatRoleStateFactory::kEventSentinel &&
          runtime.event_tree == 0x7000u + 260u &&
          runtime.event_key == pal4::CombatRoleStateFactory::kEventKey &&
          runtime.event_callback == 0x9100u &&
          runtime.words[0x9100u] ==
              pal4::CombatRoleStateFactory::kEventCallbackVtable &&
          runtime.words[0x9104u] == 0x8b694cu &&
          runtime.deleted_callback == 0x9100u &&
          runtime.delete_flags == 1u &&
          runtime.calls == std::vector<std::string>{
              "read", "allocate8", "write", "write", "manager",
              "event", "delete"},
          "show callback carries type pointer and releases temporary ownership");
    FactoryRuntime failed;
    failed.fail_allocation = true;
    failed.words[pal4::CombatRoleStateFactory::kFactorySource] = 0x8b694cu;
    Check(pal4::CombatRoleStateFactory::RegisterEvent(failed) ==
              pal4::CombatRoleStateFactory::kEventSentinel &&
          failed.event_callback == 0u && failed.deleted_callback == 0u &&
          failed.calls == std::vector<std::string>{
              "read", "allocate8", "manager", "event"},
          "show registration passes null callback on allocation failure");
}

}  // namespace

int main() {
    try {
        NineSubscriptions();
        FactoryLifecycle();
        std::cout << "combat_role_state_registration_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
