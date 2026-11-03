#include "pal4/effect_system_timed.h"

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

struct Runtime final : pal4::IEffectSystemTimedRuntime {
    static constexpr std::uint32_t kSystem = 0x1000u;
    static constexpr std::uint32_t kTree = kSystem + 244u;
    static constexpr std::uint32_t kHead = 0x2000u;
    static constexpr std::uint32_t kRoot = 0x2110u;
    static constexpr std::uint32_t kRight = 0x2130u;
    static constexpr std::uint32_t kNil = 0x9000u;
    bool enabled = true;
    bool multi_mode = false;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, float> floats;
    std::vector<std::string> calls;
    std::uint32_t inserted_parent{};
    std::uint32_t inserted_effect{};
    float inserted_lifetime{};

    Runtime() {
        words[kTree + 4u] = kHead;
        words[kHead] = kHead;
        words[kHead + 4u] = kNil;
        words[kHead + 8u] = kHead;
    }
    void Root(std::uint32_t effect, float lifetime) {
        words[kHead] = kRoot;
        words[kHead + 4u] = kRoot;
        words[kHead + 8u] = kRoot;
        words[kRoot] = kNil;
        words[kRoot + 4u] = kHead;
        words[kRoot + 8u] = kNil;
        words[kRoot + 12u] = effect;
        words[kRoot + 20u] = 1u;
        floats[kRoot + 16u] = lifetime;
    }
    void Right(std::uint32_t effect, float lifetime) {
        words[kRoot + 8u] = kRight;
        words[kHead + 8u] = kRight;
        words[kRight] = kNil;
        words[kRight + 4u] = kRoot;
        words[kRight + 8u] = kNil;
        words[kRight + 12u] = effect;
        words[kRight + 20u] = 1u;
        floats[kRight + 16u] = lifetime;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        if (address == kSystem + 4u) {
            calls.emplace_back("enabled");
            return enabled ? 1u : 0u;
        }
        Check(address == kTree + 8u, "timed insertion mode address");
        calls.emplace_back("mode");
        return multi_mode ? 1u : 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        const auto it = words.find(address);
        Check(it != words.end(), "unexpected timed tree read");
        return it->second;
    }
    float ReadFloat(std::uint32_t address) override {
        const auto it = floats.find(address);
        Check(it != floats.end(), "unexpected timer read");
        calls.emplace_back("read timer");
        return it->second;
    }
    void WriteFloat(std::uint32_t address, float value) override {
        floats[address] = value;
        calls.emplace_back("write timer");
    }
    std::uint32_t SharedNil(std::uint32_t tree) override {
        Check(tree == kTree, "timed nil tree");
        return kNil;
    }
    void InsertTimedNode(std::uint32_t tree, std::uint32_t nil,
                         std::uint32_t parent, std::uint32_t effect,
                         float lifetime) override {
        Check(tree == kTree && nil == kNil,
              "timed insertion tree and nil");
        inserted_parent = parent;
        inserted_effect = effect;
        inserted_lifetime = lifetime;
        calls.emplace_back("insert");
    }
    void UpdateTimedObject(std::uint32_t effect,
                           double seconds) override {
        Check(seconds == 0.5, "timed frame duration");
        calls.emplace_back("update " + std::to_string(effect));
    }
    void DrawTimedObject(std::uint32_t effect,
                         std::uint32_t context) override {
        Check(context == 0x5000u, "timed draw context");
        calls.emplace_back("draw " + std::to_string(effect));
    }
    void BeforeDestroyEffect(std::uint32_t effect) override {
        calls.emplace_back("before " + std::to_string(effect));
    }
    void DestroyEffect(std::uint32_t effect,
                       std::uint32_t flag) override {
        Check(flag == 1u, "timed deleting destructor flag");
        calls.emplace_back("destroy " + std::to_string(effect));
    }
    std::uint32_t EraseTimedNode(std::uint32_t tree,
                                 std::uint32_t node) override {
        Check(tree == kTree, "timed erase tree");
        calls.emplace_back("erase " + std::to_string(node));
        return kHead;
    }
};

void TestInsertBoundaries() {
    Runtime inactive;
    inactive.enabled = false;
    Check(!pal4::EffectSystem::InsertTimedEffect(
              Runtime::kSystem, 100u, 2.5f, inactive) &&
          inactive.calls == std::vector<std::string>{"enabled"},
          "disabled system does not search timed tree");

    Runtime empty;
    Check(pal4::EffectSystem::InsertTimedEffect(
              Runtime::kSystem, 100u, 2.5f, empty) &&
          empty.inserted_parent == Runtime::kHead &&
          empty.inserted_effect == 100u &&
          empty.inserted_lifetime == 2.5f &&
          empty.calls == (std::vector<std::string>{
              "enabled", "mode", "insert"}),
          "empty timed tree inserts below header");

    Runtime duplicate;
    duplicate.Root(100u, 1.0f);
    Check(!pal4::EffectSystem::InsertTimedEffect(
              Runtime::kSystem, 100u, 2.5f, duplicate) &&
          duplicate.inserted_effect == 0u,
          "unique mode rejects an existing effect pointer");

    Runtime before;
    before.Root(100u, 1.0f);
    Check(pal4::EffectSystem::InsertTimedEffect(
              Runtime::kSystem, 90u, 2.5f, before) &&
          before.inserted_parent == Runtime::kRoot,
          "new minimum inserts before the first node");

    Runtime after;
    after.Root(100u, 1.0f);
    Check(pal4::EffectSystem::InsertTimedEffect(
              Runtime::kSystem, 120u, 2.5f, after) &&
          after.inserted_parent == Runtime::kRoot,
          "greater pointer inserts on the right");

    Runtime between;
    between.Root(100u, 1.0f);
    between.Right(150u, 1.0f);
    Check(pal4::EffectSystem::InsertTimedEffect(
              Runtime::kSystem, 125u, 2.5f, between) &&
          between.inserted_parent == Runtime::kRight,
          "left insertion checks predecessor below a greater parent");

    Runtime multi;
    multi.Root(100u, 1.0f);
    multi.multi_mode = true;
    Check(pal4::EffectSystem::InsertTimedEffect(
              Runtime::kSystem, 100u, 2.5f, multi) &&
          multi.inserted_parent == Runtime::kRoot,
          "multi mode accepts a duplicate pointer");
}

void TestFrameUpdates() {
    Runtime empty;
    pal4::EffectSystem::UpdateEffectTimers(Runtime::kSystem, 0.5, empty);
    Check(empty.calls.empty(), "empty timed tree has no frame work");

    Runtime remaining;
    remaining.Root(0x4000u, 1.5f);
    pal4::EffectSystem::UpdateEffectTimers(Runtime::kSystem, 0.5,
                                           remaining);
    Check(remaining.floats.at(Runtime::kRoot + 16u) == 1.0f &&
          remaining.calls == (std::vector<std::string>{
              "update 16384", "read timer", "write timer"}),
          "positive timer updates object before decrementing and retains it");

    Runtime expired;
    expired.Root(0x4000u, 0.5f);
    pal4::EffectSystem::UpdateEffectTimers(Runtime::kSystem, 0.5,
                                           expired);
    Check(expired.floats.at(Runtime::kRoot + 16u) == 0.0f &&
          expired.calls == (std::vector<std::string>{
              "update 16384", "read timer", "write timer",
              "before 16384", "destroy 16384", "erase 8464"}),
          "zero timer destroys effect and erases its node");

    Runtime mixed;
    mixed.Root(0x4000u, 1.5f);
    mixed.Right(0x4100u, 0.5f);
    pal4::EffectSystem::UpdateEffectTimers(Runtime::kSystem, 0.5, mixed);
    Check(mixed.calls == (std::vector<std::string>{
              "update 16384", "read timer", "write timer",
              "update 16640", "read timer", "write timer",
              "before 16640", "destroy 16640", "erase 8496"}),
          "frame traversal advances through survivor and expired successor");
}

void TestDrawTraversal() {
    Runtime empty;
    pal4::EffectSystem::DrawTimedEffects(Runtime::kSystem, 0x5000u, empty);
    Check(empty.calls.empty(), "empty timed tree has no draw calls");

    Runtime populated;
    populated.Root(0x4000u, 1.0f);
    populated.Right(0x4100u, 2.0f);
    pal4::EffectSystem::DrawTimedEffects(Runtime::kSystem, 0x5000u,
                                         populated);
    Check(populated.calls == (std::vector<std::string>{
              "draw 16384", "draw 16640"}),
          "timed draw visits both objects in tree order");
}

}  // namespace

int main() {
    TestInsertBoundaries();
    TestFrameUpdates();
    TestDrawTraversal();
}
