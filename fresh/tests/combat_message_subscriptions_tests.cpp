#include "pal4/combat_message_subscriptions.h"
#include "pal4/system_event_router_tree.h"

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

constexpr auto kTree = 0x5000u;
constexpr auto kHead = 0x6000u;
constexpr auto kNil = 0x7000u;
constexpr auto kMessage = 0x4000u;
constexpr auto kKey = 0xaabbccu;

struct Runtime final : pal4::ICombatMessageSubscriptionRuntime,
                       pal4::ISystemEventRouterTreeRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> cleaned;
    std::vector<std::uint32_t> freed;
    std::uint32_t next = 0x8000u;
    std::uint32_t router_calls = 0;
    std::uint32_t registration_calls = 0;

    Runtime() {
        words[kTree + 4u] = kHead;
        words[kHead] = kHead;
        words[kHead + 4u] = kNil;
        words[kHead + 8u] = kHead;
        words[kNil + 28u] = 1u;
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
        const auto address = next;
        next += 0x100u;
        return address;
    }
    void Free(const std::uint32_t address) override {
        freed.push_back(address);
    }
    void CleanupPayload(const std::uint32_t address) override {
        cleaned.push_back(address);
    }
    std::uint32_t DeleteCallback(const std::uint32_t,
                                 const std::uint32_t) override {
        return 0u;
    }
    std::uint32_t EventKey(const std::string_view) override {
        throw std::runtime_error("combat message uses the global key pointer");
    }
    std::uint32_t MessageEventKey() override { return kKey; }
    std::uint32_t EventRouter() override {
        ++router_calls;
        return kTree;
    }
    std::uint32_t Register(
        const std::uint32_t router, const std::uint32_t mode,
        const std::uint32_t key, const std::uint32_t owner,
        const std::uint32_t callback) override {
        ++registration_calls;
        Check(router == kTree && mode == 3u && key == kKey &&
              owner == kMessage &&
              callback == pal4::CombatMessageSubscriptions::kCallback,
              "registration arguments match the IDA call");
        return pal4::SystemEventRouterTree::Register(
            router, mode, key, owner, callback, kNil, *this);
    }
    std::uint32_t FindEventGroup(
        const std::uint32_t router, const std::uint32_t key) override {
        Check(router == kTree && key == kKey,
              "removal looks up the saved event key");
        return pal4::SystemEventRouterTree::FindOrCreateGroup(
            router, key, kNil, *this);
    }
    void ReleaseOwnedSlot(const std::uint32_t slot) override {
        CleanupPayload(slot);
    }
    void FreeNode(const std::uint32_t node) override { Free(node); }
};

struct FactoryRuntime final : pal4::ICombatMessageFactoryRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t next = 0x9000u;
    bool fail_allocation = false;
    std::uint32_t event_callback = 0u;
    std::uint32_t event_tree = 0u;
    std::uint32_t event_key = 0u;
    std::uint32_t deleted_callback = 0u;
    std::uint32_t delete_flags = 0u;

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
        calls.emplace_back(bytes == 0x44u ? "allocate44" : "allocate8");
        if (fail_allocation) return 0u;
        const auto storage = next;
        next += 0x100u;
        return storage;
    }
    std::uint32_t ConstructMessage(
        const std::uint32_t storage,
        const std::uint32_t argument) override {
        calls.emplace_back("construct");
        Check(argument == 0x1234u, "factory forwards constructor argument");
        return storage + 4u;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x7000u;
    }
    std::uint32_t InsertFactory(
        const std::uint32_t tree,
        const std::uint32_t create_callback) override {
        calls.emplace_back("factory");
        Check(tree == 0x8765u &&
              create_callback == pal4::CombatMessageFactory::kCreateCallback,
              "factory registration uses cached type and create function");
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

void RegisterReplaceAndClear() {
    Runtime runtime;
    const auto first = pal4::CombatMessageSubscriptions::Register(
        kMessage, runtime);
    const auto group = pal4::SystemEventRouterTree::FindExact(
        kTree, kKey, kNil, runtime) + 16u;
    const auto head = runtime.words[group + 4u];
    auto node = runtime.words[head];
    Check(first && runtime.words[kMessage + 40u] == first &&
          runtime.words[kMessage + 44u] == kKey &&
          runtime.words[group + 8u] == 1u &&
          runtime.words[node + 12u] == first &&
          runtime.words[node + 16u] == 3u &&
          runtime.words[first + 4u] == kMessage &&
          runtime.words[first + 8u] ==
              pal4::CombatMessageSubscriptions::kCallback &&
          runtime.router_calls == 1u && runtime.registration_calls == 1u,
          "first registration stores callback and key at +40/+44");

    const auto second = pal4::CombatMessageSubscriptions::Register(
        kMessage, runtime);
    node = runtime.words[head];
    Check(second && second != first &&
          runtime.words[kMessage + 40u] == second &&
          runtime.words[group + 8u] == 1u &&
          runtime.words[node + 12u] == second &&
          runtime.cleaned.size() == 1u &&
          runtime.freed.size() == 1u &&
          runtime.router_calls == 3u && runtime.registration_calls == 2u,
          "repeat registration removes prior node before inserting next");
    Check(pal4::CombatMessageSubscriptions::Clear(kMessage, runtime) == 0u &&
          runtime.words[kMessage + 40u] == 0u &&
          runtime.words[kMessage + 44u] == 0u &&
          runtime.words[group + 8u] == 0u &&
          runtime.words[head] == head &&
          runtime.cleaned.size() == 2u &&
          runtime.freed.size() == 2u,
          "clear unlinks the matching callback and resets both slots");
    const auto calls = runtime.router_calls;
    Check(pal4::CombatMessageSubscriptions::Clear(kMessage, runtime) == 0u &&
          runtime.router_calls == calls,
          "empty subscription skips event-router lookup");
}

void MissingCallbackAndIdentity() {
    Runtime runtime;
    runtime.words[kMessage + 40u] = 0x1234u;
    runtime.words[kMessage + 44u] = kKey;
    Check(pal4::CombatMessageSubscriptions::Clear(kMessage, runtime) == 0u &&
          runtime.words[kMessage + 40u] == 0u &&
          runtime.words[kMessage + 44u] == 0u &&
          runtime.cleaned.empty() && runtime.freed.empty(),
          "missing callback clears fields without removing another node");
}

void FactoryRegistrationAndCreation() {
    FactoryRuntime runtime;
    runtime.words[pal4::CombatMessageFactory::kFactorySource] = 0x8765u;
    Check(pal4::CombatMessageFactory::GetInstanceThunk(runtime) == 0x8765u &&
          runtime.words[pal4::CombatMessageFactory::kFactoryCache] == 0x8765u &&
          runtime.calls == std::vector<std::string>{"read", "write"},
          "getter thunk caches the canonical type pointer");
    runtime.calls.clear();
    Check(pal4::CombatMessageFactory::GetInstance(runtime) == 0x8765u &&
          runtime.calls == std::vector<std::string>{"read", "write"},
          "direct getter refreshes the same cached pointer");
    runtime.calls.clear();
    Check(pal4::CombatMessageFactory::RegisterFactoryThunk(runtime) ==
              0xabcu &&
          runtime.calls ==
              std::vector<std::string>{"manager", "read", "factory"},
          "factory thunk obtains manager before inserting registration");
    runtime.calls.clear();
    runtime.fail_allocation = true;
    Check(pal4::CombatMessageFactory::Create(0x1234u, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{"allocate44"},
          "factory does not construct after allocation failure");
    runtime.calls.clear();
    runtime.fail_allocation = false;
    Check(pal4::CombatMessageFactory::Create(0x1234u, runtime) == 0x9004u &&
          runtime.calls ==
              std::vector<std::string>{"allocate44", "construct"},
          "factory allocates 68 bytes and forwards the constructor argument");
}

void EventFactoryRegistration() {
    FactoryRuntime runtime;
    Check(pal4::CombatMessageFactory::RegisterEventThunk(runtime) ==
              pal4::CombatMessageFactory::kEventSentinel &&
          runtime.event_tree == 0x7000u + 260u &&
          runtime.event_key == pal4::CombatMessageFactory::kEventKey &&
          runtime.event_callback == 0x9000u &&
          runtime.words[0x9000u] ==
              pal4::CombatMessageFactory::kEventCallbackVtable &&
          runtime.words[0x9004u] ==
              pal4::CombatMessageFactory::kShowCallback &&
          runtime.deleted_callback == 0x9000u &&
          runtime.delete_flags == 1u &&
          runtime.calls == std::vector<std::string>{
              "allocate8", "write", "write", "manager", "event", "delete"},
          "event callback is inserted then releases its temporary ownership");
    FactoryRuntime failed;
    failed.fail_allocation = true;
    Check(pal4::CombatMessageFactory::RegisterEvent(failed) ==
              pal4::CombatMessageFactory::kEventSentinel &&
          failed.event_callback == 0u && failed.deleted_callback == 0u &&
          failed.calls == std::vector<std::string>{
              "allocate8", "manager", "event"},
          "event registration passes an empty callback on allocation failure");
}

}  // namespace

int main() {
    try {
        RegisterReplaceAndClear();
        MissingCallbackAndIdentity();
        FactoryRegistrationAndCreation();
        EventFactoryRegistration();
        std::cout << "combat_message_subscriptions_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
