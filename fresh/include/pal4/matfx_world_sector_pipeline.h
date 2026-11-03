#pragma once

#include <cstdint>

namespace pal4 {

class IMatFxWorldSectorPipelineRuntime {
public:
    virtual ~IMatFxWorldSectorPipelineRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CreatePipeline() = 0;
    [[nodiscard]] virtual std::uint32_t NodeDefinitionAddress() = 0;
    virtual void SetMipmapping(std::uint32_t enabled) = 0;
    [[nodiscard]] virtual std::uint32_t LockPipeline(
        std::uint32_t pipeline) = 0;
    [[nodiscard]] virtual std::uint32_t AddFragment(
        std::uint32_t locked, std::uint32_t first,
        std::uint32_t definition_address, std::uint32_t last) = 0;
    [[nodiscard]] virtual std::uint32_t UnlockPipeline(
        std::uint32_t locked) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FindNode(
        std::uint32_t pipeline, std::uint32_t name,
        std::uint32_t first, std::uint32_t last) = 0;
    virtual void SetInstanceCallback(std::uint32_t node,
                                     std::uint32_t callback) = 0;
    virtual void SetRenderCallback(std::uint32_t node,
                                     std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyPipeline(
        std::uint32_t pipeline) = 0;
};

class MatFxWorldSectorPipeline final {
public:
    static constexpr std::uint32_t kInstanceCallback = 0x53e550u;
    static constexpr std::uint32_t kRenderCallback = 0x53e990u;

    [[nodiscard]] static std::uint32_t Create(
        IMatFxWorldSectorPipelineRuntime& runtime);
};

}  // namespace pal4
