#include "pal4/combat_event_handlers_all.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Spec {
    std::uint32_t descriptor;
    std::uint32_t mode;
    std::uint32_t callback;
};

constexpr std::array<Spec, 17> kExpected{{
    {0x8b8b88u, 3u, 0x57a910u}, {0x8b8b84u, 3u, 0x57a770u},
    {0x8b8b8cu, 1u, 0x57a920u}, {0x8b8b94u, 2u, 0x57b0a0u},
    {0x8b8b9cu, 2u, 0x57b0c0u}, {0x8b8ba8u, 2u, 0x57b0f0u},
    {0x8b8b90u, 2u, 0x57b8d0u}, {0x8b5998u, 3u, 0x57b220u},
    {0x8b599cu, 3u, 0x57b520u}, {0x8b631cu, 3u, 0x57b570u},
    {0x8b5fc8u, 3u, 0x57b570u}, {0x8b5954u, 2u, 0x57b8d0u},
    {0x8b5958u, 2u, 0x57b5e0u}, {0x8b5988u, 2u, 0x57b710u},
    {0x8b62c8u, 2u, 0x57b120u}, {0x8b5f88u, 2u, 0x57b1a0u},
    {0x8b8bb0u, 2u, 0x57b7b0u},
}};

struct Runtime final : pal4::IEventSubscriptionRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<Spec> registered;
    std::uint32_t next_handle = 0x500u;
    std::uint32_t group = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("word:" + std::to_string(address));
    }
    std::uint32_t EventKey(std::string_view) override {
        throw std::logic_error("fixed descriptor");
    }
    std::uint32_t EventRouter() override {
        calls.emplace_back("router");
        return 0x2000u;
    }
    std::uint32_t Register(std::uint32_t router, std::uint32_t mode,
                           std::uint32_t descriptor, std::uint32_t owner,
                           std::uint32_t callback) override {
        Check(router == 0x2000u && owner == 0x1000u,
              "combat event router and owner");
        registered.push_back({descriptor, mode, callback});
        calls.emplace_back("register:" + std::to_string(descriptor));
        return next_handle++;
    }
    std::uint32_t FindEventGroup(std::uint32_t router,
                                 std::uint32_t) override {
        Check(router == 0x2000u, "remove router");
        calls.emplace_back("group");
        return group;
    }
    void ReleaseOwnedSlot(std::uint32_t slot) override {
        calls.emplace_back("release:" + std::to_string(slot));
    }
    void FreeNode(std::uint32_t node) override {
        calls.emplace_back("free:" + std::to_string(node));
    }
};

void CheckBindings(const Runtime& runtime) {
    Check(runtime.registered.size() == kExpected.size(),
          "all 17 handlers registered");
    for (std::size_t index = 0u; index < kExpected.size(); ++index) {
        const auto& actual = runtime.registered[index];
        const auto& expected = kExpected[index];
        Check(actual.descriptor == expected.descriptor &&
                  actual.mode == expected.mode &&
                  actual.callback == expected.callback &&
                  runtime.words.at(0x1000u + 8u * index) ==
                      0x500u + index &&
                  runtime.words.at(0x1004u + 8u * index) ==
                      expected.descriptor,
              "fixed handler descriptor, mode, callback and slot");
    }
}

void InitializeAndConstruct() {
    Runtime runtime;
    Check(pal4::CombatEventHandlersAll::Construct(
              0x1000u, runtime) == 0x1000u,
          "constructor returns owner");
    CheckBindings(runtime);
    Check(runtime.calls[0] == "word:4096" &&
              runtime.calls[33] == "word:4228" &&
              runtime.calls[34] == "router" &&
              runtime.calls[runtime.calls.size() - 2] == "word:4228" &&
              runtime.calls.back() == "word:4224",
          "clear precedes subscriptions; last descriptor precedes handle");

    Runtime direct;
    Check(pal4::CombatEventHandlersAll::Initialize(
              0x1000u, direct) == 0x510u,
          "direct initializer returns last handle");
    CheckBindings(direct);
}

void Removal() {
    Runtime runtime;
    constexpr std::uint32_t head = 0x3000u;
    constexpr std::uint32_t node = 0x3100u;
    runtime.words[0x1000u] = 0x500u;
    runtime.words[0x1004u] = kExpected[0].descriptor;
    runtime.group = 0x4000u;
    runtime.words[0x4004u] = head;
    runtime.words[0x4008u] = 1u;
    runtime.words[head] = node;
    runtime.words[head + 4u] = node;
    runtime.words[node] = head;
    runtime.words[node + 4u] = head;
    runtime.words[node + 12u] = 0x500u;
    pal4::CombatEventHandlersAll::Remove(0x1000u, runtime);
    Check(runtime.words[head] == head &&
              runtime.words[head + 4u] == head &&
              runtime.words[0x4008u] == 0u &&
              runtime.words[0x1000u] == 0u &&
              runtime.words[0x1004u] == 0u,
          "remove unlinks matching node and clears slot");
    bool released = false;
    bool freed = false;
    for (const auto& call : runtime.calls) {
        released |= call == "release:12552";
        freed |= call == "free:12544";
    }
    Check(released && freed, "remove releases handler and node");
}

}  // namespace

int main() {
    try {
        InitializeAndConstruct();
        Removal();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
