#include "pal4/camera_world_sector_pipeline.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ICameraWorldSectorPipelineRuntime {
    std::vector<std::string> calls;
    std::uint32_t capability_level{2u};
    std::uint32_t pipeline_result{0x5000u};
    std::uint32_t lock_result{0x6000u};
    std::uint32_t fragment_result{0x7000u};

    [[nodiscard]] std::uint32_t GetCapabilities() override {
        calls.push_back("capabilities");
        return 0x4000u;
    }
    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override {
        calls.push_back("read-word");
        Check(address == 0x4098u || address == 0x9000u,
              "capability field or node name address");
        return address == 0x4098u ? capability_level : 0x9100u;
    }
    [[nodiscard]] std::uint32_t CreatePipeline() override {
        calls.push_back("create");
        return pipeline_result;
    }
    [[nodiscard]] std::uint32_t LockPipeline(
        const std::uint32_t pipeline) override {
        Check(pipeline == 0x5000u, "lock created pipeline");
        calls.push_back("lock");
        return lock_result;
    }
    [[nodiscard]] std::uint32_t GetNodeDefinition() override {
        calls.push_back("definition");
        return 0x9000u;
    }
    [[nodiscard]] std::uint32_t AddFragment(
        const std::uint32_t locked, const std::uint32_t first,
        const std::uint32_t definition,
        const std::uint32_t last) override {
        Check(locked == 0x6000u && first == 0u &&
                  definition == 0x9000u && last == 0u,
              "fragment arguments");
        calls.push_back("fragment");
        return fragment_result;
    }
    void UnlockPipeline(const std::uint32_t locked) override {
        Check(locked == fragment_result,
              "unlock uses fragment return even when zero");
        calls.push_back("unlock");
    }
    [[nodiscard]] std::uint32_t FindNode(
        const std::uint32_t pipeline, const std::uint32_t name,
        const std::uint32_t first, const std::uint32_t last) override {
        Check(pipeline == 0x5000u && name == 0x9100u &&
                  first == 0u && last == 0u, "node search arguments");
        calls.push_back("find");
        return 0xa000u;
    }
    void SetRenderCallback(const std::uint32_t node,
                             const std::uint32_t callback) override {
        Check(node == 0xa000u && callback == 0x5fd300u,
              "render geometry render callback");
        calls.push_back("callback");
    }
    void DestroyPipeline(const std::uint32_t pipeline) override {
        Check(pipeline == 0x5000u, "destroy failed locked pipeline");
        calls.push_back("destroy");
    }
};

void TestSuccess() {
    Runtime runtime;
    Check(pal4::CameraWorldSectorPipeline::Create(runtime) == 0x5000u,
          "pipeline creation result");
    Check(runtime.calls == std::vector<std::string>{
              "capabilities", "read-word", "create", "lock", "definition",
              "fragment", "unlock", "read-word", "find", "callback"},
          "pipeline assembly order");

    runtime.calls.clear();
    runtime.fragment_result = 0u;
    Check(pal4::CameraWorldSectorPipeline::Create(runtime) == 0x5000u &&
              runtime.calls.back() == "callback",
          "zero fragment result still reaches callback assignment");
}

void TestFailures() {
    Runtime runtime;
    runtime.capability_level = 1u;
    Check(pal4::CameraWorldSectorPipeline::Create(runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "capabilities", "read-word"},
          "device capability at one prevents creation");

    runtime.calls.clear();
    runtime.capability_level = 2u;
    runtime.pipeline_result = 0u;
    Check(pal4::CameraWorldSectorPipeline::Create(runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "capabilities", "read-word", "create"},
          "pipeline allocation failure stops before lock");

    runtime.calls.clear();
    runtime.pipeline_result = 0x5000u;
    runtime.lock_result = 0u;
    Check(pal4::CameraWorldSectorPipeline::Create(runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "capabilities", "read-word", "create", "lock", "destroy"},
          "lock failure destroys created pipeline");
}

}  // namespace

int main() {
    TestSuccess();
    TestFailures();
}
