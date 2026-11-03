#include "pal4/actor_instance_lifecycle.h"

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

struct Runtime final : pal4::IActorInstanceRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> trace;
    std::uint32_t ReadWord(const std::uint32_t address) {
        return words[address];
    }
    std::uint8_t ReadByte(const std::uint32_t address) {
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
    void FreeText(const std::uint32_t address) {
        trace.push_back("free-text:" + std::to_string(address));
    }
    // Original VC6 behavior is reference-only test data. Production lifecycle
    // delegates release to the host owner instead of reading text[-1].
    void ReleaseText(const std::uint32_t record) override {
        const auto text = ReadWord(record + 4u);
        if (text) {
            const auto count = ReadByte(text - 1u);
            if (count == 0u || count == 0xffu) FreeText(text - 1u);
            else WriteByte(text - 1u, static_cast<std::uint8_t>(count - 1u));
        }
        for (const auto offset : {4u, 8u, 12u}) WriteWord(record + offset, 0u);
    }
    void InitializeResourceManager(const std::uint32_t actor) override {
        trace.push_back("resource:" + std::to_string(actor));
    }
    void InitializeObjectState(const std::uint32_t state) override {
        trace.push_back("state:" + std::to_string(state));
    }
    void InitializeTga(const std::uint32_t header) override {
        trace.push_back("tga:" + std::to_string(header));
    }
    void InitializeComponent(const std::uint32_t component) override {
        trace.push_back("component:" + std::to_string(component));
    }
    void InitializeNameNode(const std::uint32_t node) override {
        trace.push_back("name:" + std::to_string(node));
    }
    void SetTextFlag(const std::uint32_t record,
                     const std::uint8_t flag) override {
        if (flag == 0u)
            Check(bytes[record] == 0u,
                  "normalized text byte precedes flag initialization");
        trace.push_back("text:" + std::to_string(record) + ":" +
                        std::to_string(flag));
    }
    void ResetEffect(const std::uint32_t handle) override {
        trace.push_back("effect:" + std::to_string(handle));
    }
    std::uint32_t DestroyComponent(const std::uint32_t component) override {
        trace.push_back("destroy-component:" + std::to_string(component));
        return 0xbeefu;
    }
    void ResetActorProperties(const std::uint32_t address) override {
        trace.push_back("reset-actor:" + std::to_string(address));
    }
    void CleanupObjectData(const std::uint32_t address) override {
        trace.push_back("cleanup-data:" + std::to_string(address));
    }
    std::uint32_t GameInstance() override {
        trace.push_back("game-instance");
        return 0x8000u;
    }
    std::uint32_t CleanupClump(
        const std::uint32_t address,
        const std::uint32_t game_context) override {
        Check(game_context == 0x8000u + 428u,
              "game resource context offset");
        trace.push_back("cleanup-clump:" + std::to_string(address));
        return 0x1234u;
    }
    void CleanupObjectState(const std::uint32_t address) override {
        trace.push_back("cleanup-state:" + std::to_string(address));
    }
    void DestroyResourceManager(const std::uint32_t address) override {
        trace.push_back("destroy-resource:" + std::to_string(address));
    }
};

void ConstructionLayout() {
    Runtime runtime;
    constexpr auto actor = 0x1000u;
    Check(pal4::ActorInstanceLifecycle::Initialize(actor, runtime) == actor &&
          runtime.trace == std::vector<std::string>{
              "resource:4096", "state:4464", "tga:4620",
              "component:4720", "name:5124", "text:5132:0",
              "text:5148:0", "text:5164:0", "component:5184",
              "name:5588", "text:5596:0", "text:5612:0", "text:5628:0"},
          "actor subobjects initialize in IDA constructor order");
    for (const auto component :
         std::array{actor + 624u, actor + 1088u}) {
        Check(runtime.words[component] == 0x84499cu &&
              runtime.words[component + 388u] == 0xffffffffu &&
              runtime.words[component + 392u] == 0u &&
              runtime.words[component + 396u] == 0xffffffffu &&
              runtime.words[component + 400u] == 0u,
              "both component record layouts match native offsets");
        for (const auto offset : std::array{412u, 428u, 444u})
            Check(runtime.bytes[component + offset] == 0u,
                  "all six undefined text bytes are normalized to zero");
    }
    Check(runtime.words[actor + 492u] == 0u &&
          runtime.bytes[actor + 496u] == 0u &&
          runtime.words[actor + 500u] == 0u &&
          runtime.words[actor + 504u] == 0u &&
          runtime.words[actor + 508u] == 0u &&
          runtime.words[actor + 512u] == 0x8449c8u &&
          runtime.words[actor] == 0x844960u &&
          runtime.words[actor + 368u] == 0x844954u,
          "top-level actor fields and final vtables match constructor");
    Check(pal4::ActorInstanceLifecycle::kMinimumSize == 1548u &&
          pal4::ActorInstanceLifecycle::EmptyErrorString()[0] == '\0',
          "constructor touches 1548 bytes and empty-string thunk is valid");
}

void LegacyTextCleanup() {
    Runtime runtime;
    constexpr auto component = 0x2000u;
    runtime.words[component + 448u] = 0x4000u;
    runtime.bytes[0x3fffu] = 0u;
    runtime.words[component + 432u] = 0x5000u;
    runtime.bytes[0x4fffu] = 3u;
    runtime.words[component + 416u] = 0x6000u;
    runtime.bytes[0x5fffu] = 0xffu;
    Check(pal4::ActorInstanceLifecycle::CleanupComponentText(
              component, runtime) == 0xbeefu &&
          runtime.trace == std::vector<std::string>{
              "free-text:16383", "free-text:24575", "effect:8596",
              "destroy-component:8192"} &&
          runtime.bytes[0x4fffu] == 2u,
          "component text releases reverse order with refcount decrement");
    for (const auto offset : std::array{412u, 428u, 444u})
        Check(runtime.words[component + offset + 4u] == 0u &&
              runtime.words[component + offset + 8u] == 0u &&
              runtime.words[component + offset + 12u] == 0u,
              "component text record is cleared after release");
}

void FrameResetAndDestruction() {
    Runtime runtime;
    constexpr auto actor = 0x1000u;
    Check(pal4::ActorInstanceLifecycle::ResetFrame(actor, runtime) ==
              0x1234u &&
          runtime.trace == std::vector<std::string>{
              "reset-actor:4720", "reset-actor:5184",
              "cleanup-data:4464", "game-instance",
              "cleanup-clump:4096"},
          "frame reset clears both child actors before game clump");
    runtime.trace.clear();
    Check(pal4::ActorInstanceLifecycle::SetFrameFlags(
              actor, 7u, runtime) == 7u &&
          runtime.words[actor + 492u] == 7u &&
          runtime.words[actor + 516u] == 7u,
          "paired frame slots receive same value");
    runtime.words[actor + 500u] = 0x7000u;
    runtime.bytes[0x6fffu] = 1u;
    pal4::ActorInstanceLifecycle::Destroy(actor, runtime);
    Check(runtime.trace == std::vector<std::string>{
              "reset-actor:4720", "reset-actor:5184",
              "cleanup-data:4464", "game-instance",
              "cleanup-clump:4096", "text:5628:1", "text:5612:1",
              "text:5596:1", "effect:5588", "destroy-component:5184",
              "text:5164:1", "text:5148:1", "text:5132:1",
              "effect:5124", "destroy-component:4720",
              "cleanup-state:4464", "destroy-resource:4096"} &&
          runtime.bytes[0x6fffu] == 0u &&
          runtime.words[actor + 500u] == 0u &&
          runtime.words[actor + 504u] == 0u &&
          runtime.words[actor + 508u] == 0u &&
          runtime.words[actor + 512u] == 0x8425a0u &&
          runtime.words[actor] == 0x844960u &&
          runtime.words[actor + 368u] == 0x844954u,
          "actor destruction reverses children then resource manager");
}

}  // namespace

int main() {
    try {
        ConstructionLayout();
        LegacyTextCleanup();
        FrameResetAndDestruction();
        std::cout << "actor_instance_lifecycle_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
