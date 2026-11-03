#include "pal4/effect_system_group_update.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, std::string_view message) {
    if (condition) return;
    std::cerr << message << '\n';
    std::exit(1);
}

struct TimerRuntime final : pal4::IEffectSystemSecondaryTimerRuntime {
    static constexpr std::uint32_t kSystem = 0x1000u;
    static constexpr std::uint32_t kTree = kSystem + 28u;
    static constexpr std::uint32_t kHead = 0x2100u;
    static constexpr std::uint32_t kNode = 0x2110u;
    static constexpr std::uint32_t kSecond = 0x2130u;
    static constexpr std::uint32_t kNil = 0x9000u;
    bool enabled = true;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, float> floats;
    std::vector<std::string> calls;

    TimerRuntime() {
        words[kTree + 4u] = kHead;
        words[kHead] = kHead;
        words[kHead + 4u] = kNil;
        words[kHead + 8u] = kHead;
    }
    void One(float remaining) {
        words[kHead] = kNode;
        words[kHead + 4u] = kNode;
        words[kHead + 8u] = kNode;
        words[kNode] = kNil;
        words[kNode + 4u] = kHead;
        words[kNode + 8u] = kNil;
        words[kNode + 12u] = 0x4000u;
        floats[kNode + 16u] = remaining;
    }
    void Two(float remaining) {
        words[kNode + 8u] = kSecond;
        words[kHead + 8u] = kSecond;
        words[kSecond] = kNil;
        words[kSecond + 4u] = kNode;
        words[kSecond + 8u] = kNil;
        words[kSecond + 12u] = 0x4100u;
        floats[kSecond + 16u] = remaining;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == kSystem + 4u, "secondary enabled address");
        calls.emplace_back("enabled");
        return enabled ? 1u : 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        const auto it = words.find(address);
        Check(it != words.end(), "unexpected secondary tree read");
        return it->second;
    }
    float ReadFloat(std::uint32_t address) override {
        const auto it = floats.find(address);
        Check(it != floats.end(), "unexpected secondary timer read");
        calls.emplace_back("read timer");
        return it->second;
    }
    void WriteFloat(std::uint32_t address, float value) override {
        floats[address] = value;
        calls.emplace_back("write timer");
    }
    std::uint32_t SharedNil(std::uint32_t tree) override {
        Check(tree == kTree, "secondary nil tree");
        return kNil;
    }
    void ResetDialog(std::uint32_t effect) override {
        calls.emplace_back("reset " + std::to_string(effect));
    }
    void ReturnEffect(std::uint32_t effect,
                      std::uint32_t mode) override {
        Check(mode == 0u, "secondary pool mode");
        calls.emplace_back("return " + std::to_string(effect));
    }
    std::uint32_t EraseNode(std::uint32_t tree,
                            std::uint32_t node) override {
        Check(tree == kTree && (node == kNode || node == kSecond),
              "secondary tree erasure");
        calls.emplace_back("erase " + std::to_string(node));
        return kHead;
    }
};

struct GroupRuntime final : pal4::IEffectSystemObjectGroupRuntime {
    static constexpr std::uint32_t kSystem = 0x1000u;
    bool enabled = true;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> updated;
    std::uint32_t enabled_reads = 0u;

    GroupRuntime() {
        for (std::uint32_t offset = 68u; offset <= 200u; offset += 12u) {
            const auto head = 0x3000u + offset;
            words[kSystem + offset + 4u] = head;
            words[head] = head;
        }
    }
    void AddOne(std::uint32_t offset) {
        const auto head = words.at(kSystem + offset + 4u);
        const auto node = 0x4000u + offset;
        words[head] = node;
        words[node] = head;
        words[node + 8u] = 0x5000u + offset;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == kSystem + 4u, "group enabled address");
        ++enabled_reads;
        return enabled ? 1u : 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        const auto it = words.find(address);
        Check(it != words.end(), "unexpected group list read");
        return it->second;
    }
    void UpdateObject(std::uint32_t object,
                      std::uint32_t first,
                      std::uint32_t second) override {
        Check(first == 0xaaaau && second == 0xbbbbu,
              "group update arguments");
        updated.push_back(object);
    }
};

void TestSecondaryTimers() {
    TimerRuntime disabled;
    disabled.enabled = false;
    pal4::EffectSystem::UpdateSecondaryTimers(
        TimerRuntime::kSystem, 0.5, disabled);
    Check(disabled.calls == std::vector<std::string>{"enabled"},
          "disabled system skips second effect tree");

    TimerRuntime empty;
    pal4::EffectSystem::UpdateSecondaryTimers(
        TimerRuntime::kSystem, 0.5, empty);
    Check(empty.calls == std::vector<std::string>{"enabled"},
          "empty second effect tree has no work");

    TimerRuntime positive;
    positive.One(0.25f);
    pal4::EffectSystem::UpdateSecondaryTimers(
        TimerRuntime::kSystem, 0.5, positive);
    Check(positive.floats.at(TimerRuntime::kNode + 16u) == -0.25f &&
          positive.calls == (std::vector<std::string>{
              "enabled", "read timer", "read timer", "write timer"}),
          "positive timer crosses zero without retiring until next frame");

    TimerRuntime expired;
    expired.One(0.0f);
    pal4::EffectSystem::UpdateSecondaryTimers(
        TimerRuntime::kSystem, 0.5, expired);
    Check(expired.calls == (std::vector<std::string>{
              "enabled", "read timer", "reset 16384",
              "return 16384", "erase 8464"}),
          "nonpositive timer resets, returns and erases effect");

    TimerRuntime mixed;
    mixed.One(1.0f);
    mixed.Two(-0.1f);
    pal4::EffectSystem::UpdateSecondaryTimers(
        TimerRuntime::kSystem, 0.5, mixed);
    Check(mixed.calls == (std::vector<std::string>{
              "enabled", "read timer", "read timer", "write timer",
              "read timer", "reset 16640", "return 16640", "erase 8496"}),
          "second effect tree advances through live and expired nodes");
}

void TestObjectGroupOrder() {
    GroupRuntime disabled;
    disabled.enabled = false;
    pal4::EffectSystem::UpdateObjectGroups(
        GroupRuntime::kSystem, 0xaaaau, 0xbbbbu, disabled);
    Check(disabled.enabled_reads == 1u && disabled.updated.empty(),
          "disabled system skips all twelve object lists");

    GroupRuntime empty;
    pal4::EffectSystem::UpdateObjectGroups(
        GroupRuntime::kSystem, 0xaaaau, 0xbbbbu, empty);
    Check(empty.enabled_reads == 1u && empty.updated.empty(),
          "empty object lists have no callbacks");

    constexpr std::uint32_t offsets[] = {
        68u, 92u, 80u, 116u, 128u, 152u,
        164u, 104u, 140u, 176u, 188u, 200u};
    GroupRuntime populated;
    std::vector<std::uint32_t> expected;
    for (const auto offset : offsets) {
        populated.AddOne(offset);
        expected.push_back(0x5000u + offset);
    }
    pal4::EffectSystem::UpdateObjectGroups(
        GroupRuntime::kSystem, 0xaaaau, 0xbbbbu, populated);
    Check(populated.updated == expected,
          "object groups call virtual update in exact IDA list order");
}

}  // namespace

int main() {
    TestSecondaryTimers();
    TestObjectGroupOrder();
}
