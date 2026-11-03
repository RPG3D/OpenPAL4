#include "pal4/effect_system_active_frame.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
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

struct Runtime final : pal4::IEffectSystemActiveFrameRuntime {
    static constexpr std::uint32_t kSystem = 0x1000u;
    static constexpr std::uint32_t kTree = kSystem + 12u;
    static constexpr std::uint32_t kHead = 0x2000u;
    static constexpr std::uint32_t kNode = 0x2110u;
    static constexpr std::uint32_t kSecondNode = 0x2130u;
    static constexpr std::uint32_t kEffect = 0x4000u;
    static constexpr std::uint32_t kSecondEffect = 0x4100u;
    static constexpr std::uint32_t kInlineHead = 0x2500u;
    static constexpr std::uint32_t kInlineNode = 0x2510u;
    static constexpr std::uint32_t kNil = 0x9000u;
    bool enabled = true;
    bool finish_during_update = false;
    bool existing_id = false;
    bool throw_on_notify = false;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;

    Runtime() {
        words[kTree + 4u] = kHead;
        words[kHead] = kNode;
        words[kHead + 4u] = kNode;
        words[kHead + 8u] = kNode;
        words[kNode] = kNil;
        words[kNode + 4u] = kHead;
        words[kNode + 8u] = kNil;
        words[kNode + 12u] = 7u;
        words[kNode + 16u] = kEffect;
        words[kNode + 20u] = 2u;
        words[kEffect + 8u] = 0u;
        bytes[kEffect + 80u] = 0u;
        words[kSystem + 212u + 4u] = kInlineHead;
        words[kInlineHead] = kInlineHead;
        words[kInlineHead + 4u] = kNil;
        words[kInlineHead + 8u] = kInlineHead;
    }
    void Empty() {
        words[kHead] = kHead;
        words[kHead + 4u] = kNil;
        words[kHead + 8u] = kHead;
    }
    void Second(std::int32_t repeats) {
        words[kNode + 8u] = kSecondNode;
        words[kHead + 8u] = kSecondNode;
        words[kSecondNode] = kNil;
        words[kSecondNode + 4u] = kNode;
        words[kSecondNode + 8u] = kNil;
        words[kSecondNode + 12u] = 8u;
        words[kSecondNode + 16u] = kSecondEffect;
        words[kSecondNode + 20u] =
            static_cast<std::uint32_t>(repeats);
        words[kSecondEffect + 8u] = 0u;
        bytes[kSecondEffect + 80u] = 0u;
    }
    void InactiveInline() {
        words[kInlineHead] = kInlineNode;
        words[kInlineHead + 4u] = kInlineNode;
        words[kInlineHead + 8u] = kInlineNode;
        words[kInlineNode] = kNil;
        words[kInlineNode + 4u] = kInlineHead;
        words[kInlineNode + 8u] = kNil;
        bytes[kInlineNode + 16u] = 0u;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        if (address == kSystem + 4u) {
            calls.emplace_back("enabled");
            return enabled ? 1u : 0u;
        }
        const auto it = bytes.find(address);
        Check(it != bytes.end(), "unexpected active effect byte read");
        if (address == kInlineNode + 16u)
            calls.emplace_back("inline inactive");
        else
            calls.emplace_back("object flag");
        return it->second;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        const auto it = words.find(address);
        Check(it != words.end(), "unexpected active effect word read");
        if (address == kSystem + 212u + 4u)
            calls.emplace_back("prune");
        return it->second;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        if (address == kNode + 20u ||
            address == kSecondNode + 20u)
            calls.emplace_back("repeat " + std::to_string(value));
        else if (address == kEffect + 12u ||
                 address == kSecondEffect + 12u)
            calls.emplace_back("owner");
        else if (address == kNode + 16u ||
                 address == kSecondNode + 16u)
            calls.emplace_back("clear object");
    }
    std::uint32_t SharedNil(std::uint32_t tree) override {
        Check(tree == kTree || tree == kSystem + 212u,
              "active tree nil source");
        return kNil;
    }
    std::uint32_t RemoveInactiveInline(std::uint32_t tree,
                                       std::uint32_t node) override {
        Check(tree == kSystem + 212u && node == kInlineNode,
              "inactive inline removal");
        calls.emplace_back("remove inline");
        words[kInlineHead] = kInlineHead;
        words[kInlineHead + 4u] = kNil;
        words[kInlineHead + 8u] = kInlineHead;
        return kInlineHead;
    }
    void UpdatePlayback(std::uint32_t effect,
                        double seconds) override {
        Check(seconds == 0.25, "active frame duration");
        calls.emplace_back("update " + std::to_string(effect));
        if (finish_during_update) words[effect + 8u] = 0u;
    }
    void StartPlayback(std::uint32_t effect) override {
        calls.emplace_back("start " + std::to_string(effect));
        words[effect + 8u] = 1u;
    }
    void PausePlayback(std::uint32_t effect) override {
        calls.emplace_back("pause " + std::to_string(effect));
    }
    void ResetDialog(std::uint32_t effect) override {
        calls.emplace_back("reset " + std::to_string(effect));
    }
    void ReturnEffect(std::uint32_t effect,
                      std::uint32_t mode) override {
        Check(mode == 0u, "active effect pool mode");
        calls.emplace_back("return " + std::to_string(effect));
    }
    pal4::EffectSystemHandle CopyHandle(
        std::uint32_t id, std::uint32_t system) override {
        Check((id == 7u || id == 8u) && system == kSystem,
              "active effect handle source");
        calls.emplace_back("copy handle");
        return {id, system};
    }
    void ResetTemporaryHandle(pal4::EffectSystemHandle& handle) override {
        Check(handle.system == kSystem, "active temporary handle");
        calls.emplace_back("reset handle");
        handle = {};
    }
    void NotifyStarted(std::uint32_t system,
                       const pal4::EffectSystemHandle& handle,
                       std::int32_t repeat_count) override {
        Check(system == kSystem && handle.system == kSystem,
              "start callback handle");
        calls.emplace_back("started " + std::to_string(repeat_count));
        if (throw_on_notify) throw std::runtime_error("started");
    }
    void NotifyFinished(std::uint32_t system,
                        const pal4::EffectSystemHandle& handle,
                        std::int32_t repeat_count) override {
        Check(system == kSystem && handle.system == kSystem,
              "finish callback handle");
        calls.emplace_back("finished " + std::to_string(repeat_count));
        if (throw_on_notify) throw std::runtime_error("finished");
    }
    std::uint32_t FindXmlAttribute(std::uint32_t xml,
                                    std::string_view key) override {
        Check(xml == kEffect + 20u && key == "ID", "restarted XML ID lookup");
        calls.emplace_back("find ID");
        return existing_id ? 0x6000u : 0u;
    }
    void SetXmlInt(std::uint32_t attribute,
                   std::uint32_t value) override {
        Check(attribute == 0x6000u && value == 7u,
              "restarted existing ID attribute");
        calls.emplace_back("set ID");
    }
    void AddXmlInt(std::uint32_t xml, std::string_view key,
                   std::uint32_t value) override {
        Check(xml == kEffect + 20u && key == "ID" && value == 7u,
              "restarted new ID attribute");
        calls.emplace_back("add ID");
    }
    std::uint32_t EraseActiveNode(std::uint32_t tree,
                                  std::uint32_t node) override {
        Check(tree == kTree &&
              (node == kNode || node == kSecondNode),
              "active tree erasure");
        calls.emplace_back("erase");
        return kHead;
    }
};

void Update(Runtime& runtime) {
    pal4::EffectSystem::UpdateActiveEffects(
        Runtime::kSystem, 0.25, runtime);
}

void TestGuardsAndInfiniteMode() {
    Runtime disabled;
    disabled.enabled = false;
    Update(disabled);
    Check(disabled.calls == std::vector<std::string>{"enabled"},
          "disabled active system does no frame work");

    Runtime empty;
    empty.Empty();
    Update(empty);
    Check(empty.calls == std::vector<std::string>{"enabled"},
          "empty active tree has no object work");

    Runtime hidden;
    hidden.bytes[Runtime::kEffect + 80u] = 1u;
    Update(hidden);
    Check(hidden.calls == (std::vector<std::string>{
              "enabled", "object flag"}),
          "object flag at +80 skips all playback work");

    Runtime started;
    started.words[Runtime::kNode + 20u] = 0xffffffffu;
    started.InactiveInline();
    started.existing_id = true;
    Update(started);
    Check(started.calls == (std::vector<std::string>{
              "enabled", "object flag", "prune", "inline inactive",
              "remove inline", "owner", "start 16384", "pause 16384",
              "find ID", "set ID", "copy handle", "started -1",
              "reset handle"}),
          "infinite mode restarts stopped effect and updates XML ID");

    Runtime playing;
    playing.words[Runtime::kNode + 20u] = 0xffffffffu;
    playing.words[Runtime::kEffect + 8u] = 1u;
    Update(playing);
    Check(playing.calls == (std::vector<std::string>{
              "enabled", "object flag", "update 16384"}),
          "infinite mode advances playing effect");

    Runtime ended;
    ended.words[Runtime::kNode + 20u] = 0xffffffffu;
    ended.words[Runtime::kEffect + 8u] = 1u;
    ended.finish_during_update = true;
    Update(ended);
    Check(ended.calls == (std::vector<std::string>{
              "enabled", "object flag", "update 16384", "prune",
              "copy handle", "finished -1", "reset handle"}),
          "infinite mode reports playback completion without pool return");
}

void TestFiniteModes() {
    Runtime zero;
    zero.words[Runtime::kNode + 20u] = 0u;
    Update(zero);
    Check(zero.calls == (std::vector<std::string>{
              "enabled", "object flag", "copy handle", "finished 0",
              "reset 16384", "return 16384", "clear object", "erase",
              "reset handle"}) &&
          zero.words.at(Runtime::kNode + 16u) == 0u,
          "zero count immediately retires and erases the effect");

    Runtime negative;
    negative.words[Runtime::kNode + 20u] = 0xfffffffeu;
    Update(negative);
    Check(negative.calls == (std::vector<std::string>{
              "enabled", "object flag"}),
          "other negative repeat counts leave effect unchanged");

    Runtime playing;
    playing.words[Runtime::kEffect + 8u] = 1u;
    Update(playing);
    Check(playing.calls == (std::vector<std::string>{
              "enabled", "object flag", "update 16384"}),
          "positive repeat count advances active playback");

    Runtime restart;
    restart.InactiveInline();
    Update(restart);
    Check(restart.words.at(Runtime::kNode + 20u) == 1u &&
          restart.calls == (std::vector<std::string>{
              "enabled", "object flag", "prune", "inline inactive",
              "remove inline", "repeat 1", "owner", "start 16384",
              "pause 16384", "copy handle", "started 1",
              "reset handle"}),
          "remaining repeat restarts after pruning inactive inline effects");

    Runtime last;
    last.words[Runtime::kNode + 20u] = 1u;
    Update(last);
    Check(last.words.at(Runtime::kNode + 20u) == 0u &&
          last.calls == (std::vector<std::string>{
              "enabled", "object flag", "prune", "repeat 0",
              "copy handle", "finished 0", "reset 16384",
              "return 16384", "clear object", "erase", "reset handle"}),
          "last repeat decrements to zero and retires object");
}

void TestTraversalAndExceptionCleanup() {
    Runtime mixed;
    mixed.words[Runtime::kEffect + 8u] = 1u;
    mixed.Second(0);
    Update(mixed);
    Check(mixed.calls == (std::vector<std::string>{
              "enabled", "object flag", "update 16384",
              "object flag", "copy handle", "finished 0",
              "reset 16640", "return 16640", "clear object", "erase",
              "reset handle"}),
          "active traversal reaches and retires a second node");

    Runtime thrown;
    thrown.words[Runtime::kNode + 20u] = 0u;
    thrown.throw_on_notify = true;
    bool caught = false;
    try {
        Update(thrown);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && thrown.calls == (std::vector<std::string>{
              "enabled", "object flag", "copy handle", "finished 0",
              "reset handle"}),
          "callback exception resets temporary handle before unwinding");
}

}  // namespace

int main() {
    TestGuardsAndInfiniteMode();
    TestFiniteModes();
    TestTraversalAndExceptionCleanup();
}
