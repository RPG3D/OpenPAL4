#include "pal4/world_manager_cleanup.h"

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
constexpr std::uint32_t kWorld = kManager + 116u;
constexpr std::uint32_t kVmSlot = 0x8f94c4u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Pipeline final : pal4::IMatFxWorldSectorPipelineRuntime {
    std::vector<std::string>* calls = nullptr;
    std::uint32_t CreatePipeline() override { return 0u; }
    std::uint32_t NodeDefinitionAddress() override { return 0u; }
    void SetMipmapping(std::uint32_t) override {}
    std::uint32_t LockPipeline(std::uint32_t) override { return 0u; }
    std::uint32_t AddFragment(std::uint32_t, std::uint32_t,
                              std::uint32_t, std::uint32_t) override {
        return 0u;
    }
    std::uint32_t UnlockPipeline(std::uint32_t) override { return 0u; }
    std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    std::uint32_t FindNode(std::uint32_t, std::uint32_t,
                            std::uint32_t, std::uint32_t) override {
        return 0u;
    }
    void SetInstanceCallback(std::uint32_t, std::uint32_t) override {}
    void SetRenderCallback(std::uint32_t, std::uint32_t) override {}
    std::uint32_t DestroyPipeline(std::uint32_t pipeline) override {
        calls->emplace_back("pipeline:" + std::to_string(pipeline));
        return 9u;
    }
};

struct Runtime final : pal4::IWorldManagerCleanupRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    Pipeline pipeline;

    Runtime() { pipeline.calls = &calls; }
    void CompactAtomicList(std::uint32_t list) override {
        calls.emplace_back("compact:" + std::to_string(list - kManager));
    }
    void CleanupWorld(std::uint32_t world,
                      std::uint32_t game_context) override {
        Check(world == kWorld && game_context == 0x8000u + 0x1acu,
              "world cleanup receives game context");
        calls.emplace_back("cleanup-world");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte:" + std::to_string(address));
    }
    void FreeTextStorage(std::uint32_t address) override {
        calls.emplace_back("free-text:" + std::to_string(address));
    }
    void ReleaseMatFxVm(std::uint32_t vm) override {
        calls.emplace_back("release-vm:" + std::to_string(vm));
    }
    std::uint32_t GameStateAddress() override { return 0u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("word:" + std::to_string(address));
    }
    std::string_view ReadCString(std::uint32_t) override { return {}; }
    std::uint32_t LoadPackage(std::string_view, std::uint32_t) override {
        return 0u;
    }
    void ProcessScript(std::uint32_t) override {}
    void CleanupAllScripts() override {}
    void DestroyScript(std::uint32_t) override {}
    std::string ConcatenateWorldBase(std::uint32_t,
                                      std::uint32_t) override { return {}; }
    std::uint32_t FinishWorldBase(std::string_view) override { return 0u; }
    std::uint32_t GameInstance() override {
        calls.emplace_back("game-instance");
        return 0x8000u;
    }
    void InitializeWorld(std::uint32_t, std::string_view,
                         std::string_view, std::string_view,
                         std::uint32_t) override {}
    void AddLight(std::uint32_t, std::array<float, 4>) override {}
    std::uint32_t OpenFile(std::string_view, std::string_view) override {
        return 0u;
    }
    void ReadFile(std::uint32_t, std::uint32_t,
                  std::uint32_t, std::uint32_t) override {}
    void CloseFile(std::uint32_t) override {}
    pal4::IMatFxWorldSectorPipelineRuntime& PipelineRuntime() override {
        return pipeline;
    }
    void BindWorldSectors(std::uint32_t, std::uint32_t,
                           std::uint32_t) override {}
    void AddClump(std::uint32_t, std::uint32_t) override {}
    void ProcessPluginAtomics(std::uint32_t,
                              std::uint32_t) override {}
    void ProcessClumpAtomics(std::uint32_t) override {}
};

std::size_t Position(const Runtime& runtime, std::string_view call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(found != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void PopulatedCleanupOrder() {
    Runtime runtime;
    runtime.words[kWorld + 228u] = 0x7000u;
    runtime.words[kManager + 352u] = 0x8001u;
    runtime.bytes[0x8000u] = 2u;
    runtime.words[kManager + 368u] = 0x9001u;
    runtime.bytes[0x9000u] = 0u;
    runtime.words[kVmSlot] = 0xa000u;
    Check(pal4::WorldManagerCleanup::Run(kManager, runtime) == 1u,
          "cleanup returns MatFx VM release result");
    Check(runtime.words[kWorld + 228u] == 0u &&
              runtime.bytes[0x8000u] == 1u &&
              runtime.words[kVmSlot] == 0u,
          "pipeline cleared, shared text decremented and VM slot cleared");
    for (const auto offset : {44u, 64u, 68u, 352u, 356u,
                              360u, 368u, 372u, 376u})
        Check(runtime.words[kManager + offset] == 0u,
              "manager state and both text descriptors cleared");
    Check(Position(runtime, "pipeline:28672") <
              Position(runtime, "compact:312") &&
              Position(runtime, "compact:312") <
              Position(runtime, "compact:328") &&
              Position(runtime, "compact:328") <
              Position(runtime, "cleanup-world") &&
              Position(runtime, "cleanup-world") <
              Position(runtime, "word:4140") &&
              Position(runtime, "word:4140") <
              Position(runtime, "byte:32768") &&
              Position(runtime, "byte:32768") <
              Position(runtime, "free-text:36864") &&
              Position(runtime, "free-text:36864") <
              Position(runtime, "release-vm:40960"),
          "pipeline, lists, world, state, text and VM cleanup order");
}

void EmptyAndSaturatedBranches() {
    Runtime runtime;
    runtime.words[kManager + 368u] = 0xb001u;
    runtime.bytes[0xb000u] = 0xffu;
    Check(pal4::WorldManagerCleanup::Run(kManager, runtime) == 1u &&
              runtime.words[kManager + 368u] == 0u &&
              Position(runtime, "free-text:45056") <
                  Position(runtime, "word:4464"),
          "saturated text freed before its descriptor clears");
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "release-vm:0") == runtime.calls.end(),
          "empty VM has no virtual release");

    Runtime single_owner;
    single_owner.words[kManager + 352u] = 0xc001u;
    single_owner.bytes[0xc000u] = 1u;
    (void)pal4::WorldManagerCleanup::Run(kManager, single_owner);
    Check(single_owner.bytes[0xc000u] == 0u &&
              std::find(single_owner.calls.begin(), single_owner.calls.end(),
                        "free-text:49152") == single_owner.calls.end(),
          "positive reference byte decremented without free");
}

}  // namespace

int main() {
    PopulatedCleanupOrder();
    EmptyAndSaturatedBranches();
}
