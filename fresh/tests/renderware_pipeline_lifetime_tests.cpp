#include "pal4/renderware_pipeline_lifetime.h"

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IRenderWarePipelineLifetimeRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        events.push_back("write:" + std::to_string(address) + ":" +
                         std::to_string(value));
        words[address] = value;
    }
    std::uint32_t DestroyPipeline(const std::uint32_t pipeline) override {
        events.push_back("destroy:" + std::to_string(pipeline));
        return 0x77u;
    }
    void FreeResourceEntry(const std::uint32_t entry) override {
        events.push_back("free:" + std::to_string(entry));
    }
};

void Run() {
    Runtime runtime;
    Check(pal4::RenderWarePipelineLifetime::DestroyIfPresent(0u, runtime) ==
              0u && runtime.events.empty(),
          "null pipeline returns zero without native call");
    Check(pal4::RenderWarePipelineLifetime::DestroyIfPresent(
              0x3000u, runtime) == 0x77u &&
              runtime.events == std::vector<std::string>{"destroy:12288"},
          "non-null pipeline forwards destroy result");
    runtime.events.clear();
    runtime.words[0x6034u] = 0x7000u;
    Check(pal4::RenderWarePipelineLifetime::BindAndReleaseResource(
              0x6000u, 0x8000u, runtime) == 0x6000u &&
              runtime.words[0x607cu] == 0x8000u &&
              runtime.events ==
                  (std::vector<std::string>{"write:24700:32768",
                                            "read:24628", "free:28672"}),
          "new pipeline is stored before resource entry is freed");
    runtime.events.clear();
    runtime.words[0x6034u] = 0u;
    Check(pal4::RenderWarePipelineLifetime::BindAndReleaseResource(
              0x6000u, 0u, runtime) == 0x6000u &&
              runtime.events.size() == 2u,
          "missing resource entry skips free");
    pal4::RenderWarePipelineLifetimeRuntimeStub stub;
    Check(pal4::RenderWarePipelineLifetime::DestroyIfPresent(0u, stub) == 0u,
          "native boundary stub is linkable");
}

}  // namespace

int main() { Run(); }
