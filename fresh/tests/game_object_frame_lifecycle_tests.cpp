#include "pal4/game_object_frame_lifecycle.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr std::uint32_t kObject = 0x1000u;
constexpr std::uint32_t kCandidate = 0x9000u;
constexpr std::uint32_t kLocalHead = 0x9100u;

struct Runtime final : pal4::IGameObjectFrameLifecycleRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> trace;
    std::vector<std::uint32_t> freed;
    std::map<std::uint32_t, std::uint32_t> next_nodes;
    std::vector<std::uint32_t> removed;
    std::uint32_t allocation_count = 0u;
    std::uint32_t fail_at = 0u;
    std::uint32_t cleaned_root = 0u;
    std::uint32_t manager_result = 0x1abu;

    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.push_back("read:" + std::to_string(address));
        if (address == 0xb100u) return 0xc100u;
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        trace.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        trace.push_back("byte:" + std::to_string(address));
        bytes[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes_requested) override {
        Check(bytes_requested == 24u, "24-byte list node allocation");
        trace.push_back("allocate");
        ++allocation_count;
        if (allocation_count == fail_at) return 0u;
        return allocation_count == 1u ? kCandidate : kLocalHead;
    }
    void Free(std::uint32_t address) override {
        trace.push_back("free");
        freed.push_back(address);
    }
    void InitEffectSystem(std::uint32_t object) override {
        Check(object == kObject, "effect base owner");
        trace.push_back("effect-init");
    }
    void ResetManager(std::uint32_t manager) override {
        Check(manager == kObject + 260u, "manager reset address");
        trace.push_back("reset-manager");
    }
    void CleanupTree(std::uint32_t holder,
                     std::uint32_t root) override {
        Check(holder == kObject + 280u, "tree holder address");
        trace.push_back("cleanup-tree");
        cleaned_root = root;
    }
    void GetFirstElement(std::uint32_t holder,
                         std::uint32_t manager) override {
        Check(holder == kObject + 280u && manager == kObject + 260u,
              "first element holder/manager");
        trace.push_back("first-element");
    }
    std::uint32_t NextNode(std::uint32_t node) override {
        trace.push_back("next-node");
        return next_nodes.at(node);
    }
    void RemoveNode(std::uint32_t holder,
                    std::uint32_t node) override {
        Check(holder == kObject + 280u, "remove node holder");
        trace.push_back("remove-node");
        removed.push_back(node);
    }
    std::uint32_t DestroyEffectSystem(std::uint32_t object) override {
        Check(object == kObject, "effect destructor owner");
        trace.push_back("effect-destroy");
        return 0xbeefu;
    }
    void DisableEffectSystem(std::uint32_t object) override {
        Check(object == kObject, "effect disable owner");
        trace.push_back("effect-disable");
    }
    std::uint32_t GameInstance() override {
        trace.push_back("game-instance");
        return 0xa100u;
    }
    std::uint32_t CombatSystem() override {
        trace.push_back("combat-system");
        return 0xa200u;
    }
    std::uint32_t CurrentCameraSlot(std::uint32_t combat) override {
        Check(combat == 0xa200u, "combat camera owner");
        trace.push_back("camera-slot");
        return 0xb100u;
    }
    void EnableEffectSystem(std::uint32_t object,
                            std::uint32_t camera,
                            std::uint32_t game_property) override {
        Check(object == kObject && camera == 0xc100u &&
                  game_property == 0xa100u + 428u,
              "enable effect arguments");
        trace.push_back("effect-enable");
    }
    std::uint32_t InitializeManagers(std::uint32_t manager) override {
        Check(manager == kObject + 260u, "manager initialize address");
        trace.push_back("manager-init");
        return manager_result;
    }
};

std::size_t Find(const Runtime& runtime, const std::string& value) {
    const auto it = std::find(runtime.trace.begin(), runtime.trace.end(),
                              value);
    Check(it != runtime.trace.end(), "required constructor operation");
    return static_cast<std::size_t>(it - runtime.trace.begin());
}

void CheckObjectFields(const Runtime& runtime) {
    for (auto offset : {264u, 268u, 272u, 276u, 292u, 296u})
        Check(runtime.words.at(kObject + offset) == 0u,
              "zeroed object word");
    for (auto offset : {260u, 280u, 281u, 288u})
        Check(runtime.bytes.at(kObject + offset) == 0u,
              "zeroed object byte");
    Check(runtime.words.at(kObject) ==
              pal4::GameObjectFrameLifecycle::kVtable &&
              runtime.words.at(kObject + 284u) == kLocalHead &&
              runtime.words.at(kObject + 300u) ==
                  pal4::GameObjectFrameLifecycle::kFrameCallback,
          "object vtable and local list pointer");
}

void NewSharedTree() {
    Runtime runtime;
    Check(pal4::GameObjectFrameLifecycle::Construct(kObject, runtime) ==
              kObject &&
              runtime.trace.front() == "effect-init" &&
              runtime.allocation_count == 2u && runtime.freed.empty() &&
              runtime.words[pal4::GameObjectFrameLifecycle::kSharedTreeHead] ==
                  kCandidate &&
              runtime.words[pal4::GameObjectFrameLifecycle::kSharedTreeCount] ==
                  1u &&
              runtime.words[kCandidate] == 0u &&
              runtime.words[kCandidate + 4u] == 0u &&
              runtime.words[kCandidate + 8u] == 0u &&
              runtime.words[kCandidate + 20u] == 1u &&
              runtime.words[kLocalHead] == kLocalHead &&
              runtime.words[kLocalHead + 4u] == kCandidate &&
              runtime.words[kLocalHead + 8u] == kLocalHead &&
              runtime.words[kLocalHead + 20u] == 0u,
          "first object establishes global tree and local circular head");
    CheckObjectFields(runtime);
    Check(Find(runtime, "write:" + std::to_string(kObject)) <
              Find(runtime, "write:" + std::to_string(kObject + 300u)) &&
              Find(runtime, "write:" + std::to_string(kObject + 300u)) <
              Find(runtime, "write:" + std::to_string(kObject + 296u)),
          "vtable precedes handler and final field reset follows");
}

void ExistingSharedTree() {
    Runtime runtime;
    runtime.words[pal4::GameObjectFrameLifecycle::kSharedTreeHead] = 0x8800u;
    runtime.words[pal4::GameObjectFrameLifecycle::kSharedTreeCount] = 7u;
    Check(pal4::GameObjectFrameLifecycle::Construct(kObject, runtime) ==
              kObject &&
              runtime.freed == std::vector<std::uint32_t>{kCandidate} &&
              runtime.words[pal4::GameObjectFrameLifecycle::kSharedTreeHead] ==
                  0x8800u &&
              runtime.words[pal4::GameObjectFrameLifecycle::kSharedTreeCount] ==
                  8u &&
              runtime.words[kLocalHead + 4u] == 0x8800u,
          "existing shared tree frees speculative head and increments count");
    CheckObjectFields(runtime);
}

void AllocationFailures() {
    Runtime first;
    first.fail_at = 1u;
    Check(pal4::GameObjectFrameLifecycle::Construct(kObject, first) == 0u &&
              first.allocation_count == 1u &&
              first.words[pal4::GameObjectFrameLifecycle::kSharedTreeCount] ==
                  0u &&
              first.words.find(kObject + 300u) == first.words.end(),
          "failed first allocation exits before shared tree changes");

    Runtime second;
    second.fail_at = 2u;
    Check(pal4::GameObjectFrameLifecycle::Construct(kObject, second) == 0u &&
              second.allocation_count == 2u &&
              second.words[pal4::GameObjectFrameLifecycle::kSharedTreeHead] ==
                  kCandidate &&
              second.words[pal4::GameObjectFrameLifecycle::kSharedTreeCount] ==
                  1u &&
              second.words.find(kObject + 300u) == second.words.end(),
          "failed local head allocation avoids native null dereference");
}

void EmptyAndLastObjectCleanup() {
    Runtime runtime;
    Check(pal4::GameObjectFrameLifecycle::Construct(kObject, runtime) ==
              kObject,
          "empty cleanup fixture");
    runtime.trace.clear();
    Check(pal4::GameObjectFrameLifecycle::Destroy(kObject, runtime) ==
              0xbeefu &&
              runtime.freed == std::vector<std::uint32_t>{
                  kLocalHead, kCandidate} &&
              runtime.words[kObject + 284u] == 0u &&
              runtime.words[kObject + 292u] == 0u &&
              runtime.words[pal4::GameObjectFrameLifecycle::kSharedTreeHead] ==
                  0u &&
              runtime.words[pal4::GameObjectFrameLifecycle::kSharedTreeCount] ==
                  0u &&
              runtime.removed.empty() &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "cleanup-tree") == runtime.trace.end() &&
              runtime.trace.back() == "effect-destroy" &&
              std::count(runtime.trace.begin(), runtime.trace.end(),
                         "reset-manager") == 2 &&
              Find(runtime, "reset-manager") <
                  Find(runtime, "effect-disable") &&
              Find(runtime, "effect-disable") < Find(runtime, "free"),
          "empty list releases local and last shared head before effect base");
}

void PopulatedTreeCleanup() {
    Runtime runtime;
    Check(pal4::GameObjectFrameLifecycle::Construct(kObject, runtime) ==
              kObject,
          "tree cleanup fixture");
    runtime.words[kObject + 292u] = 2u;
    runtime.words[kLocalHead + 4u] = 0xa100u;
    runtime.trace.clear();
    Check(pal4::GameObjectFrameLifecycle::Destroy(kObject, runtime) ==
              0xbeefu &&
              runtime.cleaned_root == 0xa100u &&
              Find(runtime, "cleanup-tree") <
                  Find(runtime, "first-element") &&
              Find(runtime, "first-element") < Find(runtime, "free") &&
              runtime.freed == std::vector<std::uint32_t>{
                  kLocalHead, kCandidate} &&
              runtime.words[kLocalHead] == kLocalHead &&
              runtime.words[kLocalHead + 8u] == kLocalHead,
          "populated tree resets links before releasing local head");
}

void NodeWalkAndSharedRetention() {
    Runtime runtime;
    Check(pal4::GameObjectFrameLifecycle::Construct(kObject, runtime) ==
              kObject,
          "list walk fixture");
    runtime.words[kObject + 292u] = 0u;
    runtime.words[kLocalHead] = 0xa100u;
    runtime.next_nodes[0xa100u] = 0xa200u;
    runtime.next_nodes[0xa200u] = kLocalHead;
    runtime.words[pal4::GameObjectFrameLifecycle::kSharedTreeCount] = 2u;
    runtime.trace.clear();
    Check(pal4::GameObjectFrameLifecycle::Destroy(kObject, runtime) ==
              0xbeefu &&
              runtime.removed == std::vector<std::uint32_t>{
                  0xa100u, 0xa200u} &&
              runtime.freed == std::vector<std::uint32_t>{kLocalHead} &&
              runtime.words[pal4::GameObjectFrameLifecycle::kSharedTreeCount] ==
                  1u &&
              runtime.words[pal4::GameObjectFrameLifecycle::kSharedTreeHead] ==
                  kCandidate &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "cleanup-tree") == runtime.trace.end(),
          "zero-count list walks nodes and retains shared head for peer");
}

void InitializeAfterConstruction() {
    Runtime runtime;
    Check(pal4::GameObjectFrameLifecycle::Initialize(kObject, runtime) ==
              0xabu &&
              runtime.trace == std::vector<std::string>{
                  "effect-disable", "game-instance", "combat-system",
                  "camera-slot", "read:45312", "effect-enable",
                  "manager-init"},
          "effect initialization order and low-byte manager result");
    runtime.manager_result = 0x200u;
    runtime.trace.clear();
    Check(pal4::GameObjectFrameLifecycle::Initialize(kObject, runtime) ==
              0u && runtime.trace.back() == "manager-init",
          "manager return is truncated to one byte");
}

void DeinitializeHelperOrder() {
    Runtime runtime;
    pal4::GameObjectFrameLifecycle::DeinitializeHelper(kObject, runtime);
    Check(runtime.trace == std::vector<std::string>{
              "reset-manager", "effect-disable"},
          "helper resets manager before disabling effect system");
}

void DirectUpdateHandlerStore() {
    Runtime runtime;
    Check(pal4::GameObjectFrameLifecycle::SetUpdateHandler(
              kObject, 0x12345678u, runtime) == 0x12345678u &&
              runtime.words[kObject + 300u] == 0x12345678u &&
              runtime.trace == std::vector<std::string>{
                  "write:" + std::to_string(kObject + 300u)},
          "handler setter stores callback at +0x12c and returns it");
    Check(pal4::GameObjectFrameLifecycle::SetUpdateHandler(
              kObject, 0u, runtime) == 0u &&
              runtime.words[kObject + 300u] == 0u,
          "handler setter accepts a null callback");
}

void DirectManagerTreeCleanup() {
    Runtime runtime;
    Check(pal4::GameObjectFrameLifecycle::Construct(kObject, runtime) ==
              kObject,
          "direct tree cleanup fixture");
    runtime.trace.clear();
    pal4::GameObjectFrameLifecycle::DestroyManagerTree(
        kObject + 260u, runtime);
    Check(runtime.trace.front() == "reset-manager" &&
              runtime.trace.back() == "free" &&
              runtime.freed == std::vector<std::uint32_t>{
                  kLocalHead, kCandidate} &&
              runtime.words[kObject + 284u] == 0u &&
              runtime.words[kObject + 292u] == 0u &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "effect-destroy") == runtime.trace.end() &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "effect-disable") == runtime.trace.end(),
          "standalone tree manager stops before effect-base destruction");
}

void HostCatalogLifecycle() {
    Runtime runtime;
    runtime.words[pal4::GameObjectFrameLifecycle::kSharedTreeHead]=0x4321u;
    runtime.words[pal4::GameObjectFrameLifecycle::kSharedTreeCount]=7u;
    Check(pal4::GameObjectFrameLifecycle::ConstructHostManaged(kObject,runtime)==kObject &&
          !runtime.allocation_count && runtime.freed.empty() &&
          runtime.words[kObject+284u]==0u && runtime.words[kObject+292u]==0u &&
          runtime.words[kObject+300u]==pal4::GameObjectFrameLifecycle::kFrameCallback,
          "host catalog constructs original fields/callback without private tree allocation");
    runtime.trace.clear();
    Check(pal4::GameObjectFrameLifecycle::DestroyHostManaged(kObject,runtime)==0xbeefu &&
          runtime.trace==std::vector<std::string>{"write:"+std::to_string(kObject),
              "reset-manager","effect-disable","reset-manager","effect-destroy"} &&
          runtime.words[pal4::GameObjectFrameLifecycle::kSharedTreeHead]==0x4321u &&
          runtime.words[pal4::GameObjectFrameLifecycle::kSharedTreeCount]==7u && runtime.freed.empty(),
          "host destruction preserves original ordering without touching legacy shared tree owners");
}

}  // namespace

int main() {
    try {
        NewSharedTree();
        ExistingSharedTree();
        AllocationFailures();
        EmptyAndLastObjectCleanup();
        PopulatedTreeCleanup();
        NodeWalkAndSharedRetention();
        InitializeAfterConstruction();
        DeinitializeHelperOrder();
        DirectUpdateHandlerStore();
        DirectManagerTreeCleanup();
        HostCatalogLifecycle();
        std::cout << "game_object_frame_lifecycle_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
