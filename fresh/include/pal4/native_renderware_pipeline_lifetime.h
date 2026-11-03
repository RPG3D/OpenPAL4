#pragma once

#include "pal4/renderware_pipeline_lifetime.h"

namespace pal4 {

// Engine-bound public SDK services for the game's pipeline lifetime helpers.
class NativeRenderWarePipelineLifetime final
    : public IRenderWarePipelineLifetimeRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    // Returns the public SDK macro's result, not the original void body's EAX.
    [[nodiscard]] std::uint32_t DestroyPipeline(std::uint32_t pipeline) override;
    void FreeResourceEntry(std::uint32_t entry) override;
};

}  // namespace pal4
