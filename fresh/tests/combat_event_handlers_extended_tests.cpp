#include "pal4/combat_event_handlers_extended.h"

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

constexpr std::array<Spec, 8> kExpected{{
    {0x8b5f9cu, 2u, 0x57c370u},
    {0x8b62f0u, 2u, 0x57c290u},
    {0x8b62d0u, 2u, 0x57c440u},
    {0x8b62ccu, 2u, 0x57c4b0u},
    {0x8b62dcu, 2u, 0x57c6c0u},
    {0x8b8b7cu, 2u, 0x57c840u},
    {0x8b8b8cu, 1u, 0x57c6b0u},
    {0x8b8b74u, 1u, 0x57c920u},
}};

struct Runtime final : pal4::ICombatEventHandlersExtendedRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<Spec> registered;
    std::uint32_t next_handle = 0x500u;
    std::uint32_t group = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("word:" + std::to_string(address));
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte:" + std::to_string(address));
    }
    std::uint32_t EventKey(std::string_view) override {
        throw std::logic_error("fixed descriptor");
    }
    std::uint32_t EventRouter() override {
        calls.emplace_back("router");
        return 0x2000u;
    }
    std::uint32_t Register(std::uint32_t router,
                           std::uint32_t mode,
                           std::uint32_t descriptor,
                           std::uint32_t owner,
                           std::uint32_t callback) override {
        Check(router == 0x2000u && owner == 0x1000u,
              "router and owner identity");
        calls.emplace_back("register:" + std::to_string(descriptor));
        registered.push_back({descriptor, mode, callback});
        return next_handle++;
    }
    std::uint32_t FindEventGroup(std::uint32_t router,
                                 std::uint32_t) override {
        Check(router == 0x2000u, "remove router identity");
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
          "all eight handlers registered");
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
              "descriptor, mode, callback and stored handle");
    }
}

void InitializeAndConstruct() {
    Runtime runtime;
    Check(pal4::CombatEventHandlersExtended::Construct(
              0x1000u, runtime) == 0x1000u,
          "constructor returns owner");
    CheckBindings(runtime);
    Check(runtime.calls[0] == "word:4096" &&
              runtime.calls[17] == "word:4164" &&
              runtime.calls[18] == "router" &&
              runtime.calls[runtime.calls.size() - 3] == "word:4156" &&
              runtime.calls[runtime.calls.size() - 2] == "word:4152" &&
              runtime.calls.back() == "byte:4168" &&
              runtime.bytes[0x1000u + 72u] == 0u,
          "clear 18 words, last descriptor before handle, then byte flag");

    Runtime direct;
    Check(pal4::CombatEventHandlersExtended::Initialize(
              0x1000u, direct) == 0x507u,
          "initializer returns final registration handle");
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
    pal4::CombatEventHandlersExtended::Remove(0x1000u, runtime);
    Check(runtime.words[head] == head &&
              runtime.words[head + 4u] == head &&
              runtime.words[0x4008u] == 0u &&
              runtime.words[0x1000u] == 0u &&
              runtime.words[0x1004u] == 0u,
          "remove unlinks node, decrements count and clears first slot");
    bool released = false;
    bool freed = false;
    for (const auto& call : runtime.calls) {
        released |= call == "release:12552";
        freed |= call == "free:12544";
    }
    Check(released && freed,
          "remove releases owned payload and frees the matched node");
}

}  // namespace

int main() {
    try {
        InitializeAndConstruct();
        Removal();
        std::cout << "combat event handlers extended tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
