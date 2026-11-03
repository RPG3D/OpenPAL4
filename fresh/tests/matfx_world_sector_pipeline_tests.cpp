#include "pal4/matfx_world_sector_pipeline.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IMatFxWorldSectorPipelineRuntime {
    std::vector<std::string> calls;
    std::uint32_t pipeline = 0x100u;
    std::uint32_t locked = 0x200u;
    std::uint32_t fragment = 1u;
    std::uint32_t unlocked = 1u;

    std::uint32_t CreatePipeline() override {
        calls.emplace_back("create");
        return pipeline;
    }
    std::uint32_t NodeDefinitionAddress() override {
        calls.emplace_back("definition");
        return 0x8d5c78u;
    }
    void SetMipmapping(std::uint32_t enabled) override {
        Check(enabled == 0u, "mipmapping disabled");
        calls.emplace_back("mipmapping");
    }
    std::uint32_t LockPipeline(std::uint32_t value) override {
        Check(value == pipeline, "lock pipeline");
        calls.emplace_back("lock");
        return locked;
    }
    std::uint32_t AddFragment(std::uint32_t value, std::uint32_t first,
                              std::uint32_t definition,
                              std::uint32_t last) override {
        Check(value == locked && first == 0u &&
                  definition == 0x8d5c78u && last == 0u,
              "fragment uses definition address");
        calls.emplace_back("fragment");
        return fragment;
    }
    std::uint32_t UnlockPipeline(std::uint32_t value) override {
        Check(value == locked, "unlock pipeline");
        calls.emplace_back("unlock");
        return unlocked;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x8d5c78u, "read definition pointer");
        calls.emplace_back("read-name");
        return 0x333u;
    }
    std::uint32_t FindNode(std::uint32_t value, std::uint32_t name,
                           std::uint32_t first, std::uint32_t last) override {
        Check(value == pipeline && name == 0x333u &&
                  first == 0u && last == 0u, "node lookup arguments");
        calls.emplace_back("find");
        return 0x444u;
    }
    void SetInstanceCallback(std::uint32_t node,
                             std::uint32_t callback) override {
        Check(node == 0x444u &&
                  callback == pal4::MatFxWorldSectorPipeline::kInstanceCallback,
              "instance callback address");
        calls.emplace_back("instance");
    }
    void SetRenderCallback(std::uint32_t node,
                             std::uint32_t callback) override {
        Check(node == 0x444u &&
                  callback == pal4::MatFxWorldSectorPipeline::kRenderCallback,
              "render callback address");
        calls.emplace_back("render");
    }
    std::uint32_t DestroyPipeline(std::uint32_t value) override {
        Check(value == pipeline, "destroy pipeline");
        calls.emplace_back("destroy");
        return 0u;
    }
};

void CreationFailure() {
    Runtime runtime;
    runtime.pipeline = 0u;
    Check(pal4::MatFxWorldSectorPipeline::Create(runtime) == 0u &&
              runtime.calls ==
                  (std::vector<std::string>{"create", "definition",
                                            "mipmapping"}),
          "failed create still obtains definition and disables mipmapping");
}

void PipelineStageFailures() {
    Runtime runtime;
    runtime.locked = 0u;
    Check(pal4::MatFxWorldSectorPipeline::Create(runtime) == 0u &&
              runtime.calls ==
                  (std::vector<std::string>{"create", "definition",
                                            "mipmapping", "lock", "destroy"}),
          "lock failure destroys pipeline");
    runtime.calls.clear();
    runtime.locked = 0x200u;
    runtime.fragment = 0u;
    Check(pal4::MatFxWorldSectorPipeline::Create(runtime) == 0u &&
              runtime.calls ==
                  (std::vector<std::string>{"create", "definition",
                                            "mipmapping", "lock", "fragment",
                                            "destroy"}),
          "fragment failure skips unlock and destroys pipeline");
    runtime.calls.clear();
    runtime.fragment = 1u;
    runtime.unlocked = 0u;
    Check(pal4::MatFxWorldSectorPipeline::Create(runtime) == 0u &&
              runtime.calls ==
                  (std::vector<std::string>{"create", "definition",
                                            "mipmapping", "lock", "fragment",
                                            "unlock", "destroy"}),
          "unlock failure destroys pipeline");
}

void SuccessfulCreation() {
    Runtime runtime;
    Check(pal4::MatFxWorldSectorPipeline::Create(runtime) == 0x100u &&
              runtime.calls ==
                  (std::vector<std::string>{"create", "definition",
                                            "mipmapping", "lock", "fragment",
                                            "unlock", "read-name", "find",
                                            "instance", "render"}),
          "successful pipeline binds both MatFx callbacks in order");
}

}  // namespace

int main() {
    CreationFailure();
    PipelineStageFailures();
    SuccessfulCreation();
}
