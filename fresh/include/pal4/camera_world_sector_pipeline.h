#pragma once

#include <cstdint>

namespace pal4 {

class ICameraWorldSectorPipelineRuntime {
public:
    virtual ~ICameraWorldSectorPipelineRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetCapabilities() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CreatePipeline() = 0;
    [[nodiscard]] virtual std::uint32_t LockPipeline(
        std::uint32_t pipeline) = 0;
    [[nodiscard]] virtual std::uint32_t GetNodeDefinition() = 0;
    [[nodiscard]] virtual std::uint32_t AddFragment(
        std::uint32_t locked, std::uint32_t first,
        std::uint32_t definition, std::uint32_t last) = 0;
    virtual void UnlockPipeline(std::uint32_t locked) = 0;
    [[nodiscard]] virtual std::uint32_t FindNode(
        std::uint32_t pipeline, std::uint32_t name,
        std::uint32_t first, std::uint32_t last) = 0;
    virtual void SetRenderCallback(std::uint32_t node,
                                     std::uint32_t callback) = 0;
    virtual void DestroyPipeline(std::uint32_t pipeline) = 0;
};

class CameraWorldSectorPipeline final {
public:
    static constexpr std::uint32_t kRenderCallback = 0x5fd300u;

    // IDA 0x5fd280.
    [[nodiscard]] static std::uint32_t Create(
        ICameraWorldSectorPipelineRuntime& runtime);
};

}  // namespace pal4
