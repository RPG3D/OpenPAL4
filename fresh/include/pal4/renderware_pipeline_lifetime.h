#pragma once

#include <cstdint>

namespace pal4 {

class IRenderWarePipelineLifetimeRuntime {
public:
    virtual ~IRenderWarePipelineLifetimeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyPipeline(
        std::uint32_t pipeline) = 0;
    virtual void FreeResourceEntry(std::uint32_t entry) = 0;
};

class RenderWarePipelineLifetimeRuntimeStub final
    : public IRenderWarePipelineLifetimeRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t DestroyPipeline(std::uint32_t) override {
        return 0u;
    }
    void FreeResourceEntry(std::uint32_t) override {}
};

class RenderWarePipelineLifetime final {
public:
    // IDA 0x51db10: null-preserving pipeline destructor thunk.
    [[nodiscard]] static std::uint32_t DestroyIfPresent(
        std::uint32_t pipeline, IRenderWarePipelineLifetimeRuntime& runtime);
    // IDA 0x51db30: world-sector pipeline pointer and resource entry reset.
    [[nodiscard]] static std::uint32_t BindAndReleaseResource(
        std::uint32_t object, std::uint32_t pipeline,
        IRenderWarePipelineLifetimeRuntime& runtime);
};

}  // namespace pal4
