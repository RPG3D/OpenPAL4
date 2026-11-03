#include "pal4/world_manager_lifecycle.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kManager = 0x1000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IWorldManagerLifecycleRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool fail_subscription = false;
    bool fail_scene = false;
    std::uint32_t subscriptions = 0u;

    void ConstructBaseManager(std::uint32_t address) override {
        Check(address == kManager, "base manager address");
        calls.emplace_back("manager-construct");
    }
    void DestroyBaseManager(std::uint32_t address) override {
        Check(address == kManager, "base manager destroy address");
        calls.emplace_back("manager-destroy");
    }
    void CleanupManager(std::uint32_t address) override {
        Check(address == kManager, "manager cleanup address");
        calls.emplace_back("cleanup");
    }
    void ConstructBaseWorld(std::uint32_t address) override {
        Check(address == kManager + 116u, "world base address");
        calls.emplace_back("world-base-construct");
        if (fail_scene) throw std::runtime_error("world construction failed");
    }
    void DestroyBaseWorld(std::uint32_t address) override {
        Check(address == kManager + 116u, "world base destroy address");
        calls.emplace_back("world-base-destroy");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("word:" + std::to_string(address - kManager));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte:" + std::to_string(address));
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("world-free:" + std::to_string(address));
    }
    void FreeTextStorage(std::uint32_t address) override {
        calls.emplace_back("text-free:" + std::to_string(address));
    }
    std::uint32_t EventKey(std::string_view name) override {
        constexpr std::array<std::string_view, 4> names{
            "re_join_scene", "re_leave_scene",
            "me_join_scene", "me_leave_scene"};
        const auto found = std::find(names.begin(), names.end(), name);
        Check(found != names.end(), "event key name");
        return 0x2000u + static_cast<std::uint32_t>(found - names.begin());
    }
    std::uint32_t EventRouter() override { return 0x4000u; }
    std::uint32_t Subscribe(std::uint32_t router, std::uint32_t mode,
                            std::uint32_t key, std::uint32_t owner,
                            std::uint32_t callback) override {
        Check(router == 0x4000u && mode == 3u && owner == kManager &&
                  key == 0x2000u + subscriptions &&
                  callback == (subscriptions % 2u == 0u ?
                      pal4::WorldManagerEventSubscriptions::kJoinCallback :
                      pal4::WorldManagerEventSubscriptions::kLeaveCallback),
              "subscription identity and order");
        calls.emplace_back("subscribe:" + std::to_string(subscriptions));
        if (fail_subscription) throw std::runtime_error("subscription failed");
        return 0x3000u + subscriptions++;
    }
    std::uint32_t FindEventGroup(std::uint32_t, std::uint32_t) override {
        return 0u;
    }
    void ReleaseOwnedSlot(std::uint32_t) override {}
    void FreeNode(std::uint32_t) override {}
};

std::size_t Position(const Runtime& runtime, std::string_view call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(found != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void ConstructAndGetActorData() {
    Runtime runtime;
    Check(pal4::WorldManagerLifecycle::Construct(kManager, runtime) == kManager,
          "constructor returns manager");
    Check(runtime.subscriptions == 4u, "all four events registered");
    for (std::uint32_t offset = 84u; offset <= 112u; offset += 8u) {
        const auto index = (offset - 84u) / 8u;
        Check(runtime.words[kManager + offset] == 0x3000u + index &&
                  runtime.words[kManager + offset + 4u] == 0x2000u + index,
              "event slots contain handles and keys");
    }
    for (const auto offset : {352u, 356u, 360u, 368u, 372u, 376u})
        Check(runtime.words[kManager + offset] == 0u,
              "text descriptors start empty");
    Check(runtime.bytes[kManager + 348u] == 0u &&
              runtime.bytes[kManager + 364u] == 0u,
          "indeterminate native stack bytes defined as zero");
    Check(Position(runtime, "manager-construct") <
              Position(runtime, "word:84") &&
              Position(runtime, "word:112") <
              Position(runtime, "world-base-construct") &&
              Position(runtime, "world-base-construct") <
              Position(runtime, "word:352") &&
              Position(runtime, "word:376") <
              Position(runtime, "subscribe:0"),
          "constructor member order");
    Check(pal4::WorldManagerLifecycle::ActorData(kManager) == kManager + 0x148u,
          "actor-data member address");
}

void DestroyTextReferenceCases() {
    Runtime runtime;
    runtime.words[kManager + 368u] = 0x8001u;
    runtime.bytes[0x8000u] = 0xffu;
    runtime.words[kManager + 352u] = 0x9001u;
    runtime.bytes[0x9000u] = 2u;
    pal4::WorldManagerLifecycle::Destroy(kManager, runtime);
    Check(runtime.bytes[0x9000u] == 1u,
          "shared first text reference decremented");
    Check(Position(runtime, "cleanup") <
              Position(runtime, "text-free:32768") &&
              Position(runtime, "text-free:32768") <
              Position(runtime, "byte:36864") &&
              Position(runtime, "byte:36864") <
              Position(runtime, "world-base-destroy") &&
              Position(runtime, "world-base-destroy") <
              Position(runtime, "manager-destroy"),
          "cleanup, reverse text release, world and base order");
    for (const auto offset : {368u, 372u, 376u, 352u, 356u, 360u})
        Check(runtime.words[kManager + offset] == 0u,
              "all text words cleared");

    Runtime zero_ref;
    zero_ref.words[kManager + 368u] = 0xa001u;
    zero_ref.bytes[0xa000u] = 0u;
    pal4::WorldManagerLifecycle::Destroy(kManager, zero_ref);
    Check(Position(zero_ref, "text-free:40960") <
              Position(zero_ref, "world-base-destroy"),
          "zero reference byte frees header");

    Runtime null_text;
    pal4::WorldManagerLifecycle::Destroy(kManager, null_text);
    Check(std::none_of(null_text.calls.begin(), null_text.calls.end(),
                       [](const std::string& call) {
                           return call.rfind("text-free:", 0u) == 0u;
                       }),
          "null text has no storage release");
}

void FailedSubscriptionUnwindsMembers() {
    Runtime runtime;
    runtime.fail_subscription = true;
    bool caught = false;
    try {
        (void)pal4::WorldManagerLifecycle::Construct(kManager, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && Position(runtime, "subscribe:0") <
              Position(runtime, "world-base-destroy") &&
              Position(runtime, "world-base-destroy") <
              Position(runtime, "manager-destroy"),
          "failed registration unwinds world and base");

    Runtime failed_scene;
    failed_scene.fail_scene = true;
    caught = false;
    try {
        (void)pal4::WorldManagerLifecycle::Construct(kManager, failed_scene);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && Position(failed_scene, "world-base-construct") <
              Position(failed_scene, "manager-destroy") &&
              std::find(failed_scene.calls.begin(), failed_scene.calls.end(),
                        "world-base-destroy") == failed_scene.calls.end(),
          "failed world construction unwinds only completed base");
}

}  // namespace

int main() {
    ConstructAndGetActorData();
    DestroyTextReferenceCases();
    FailedSubscriptionUnwindsMembers();
}
