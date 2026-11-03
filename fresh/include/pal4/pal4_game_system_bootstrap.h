#pragma once

#include <cstdint>

namespace pal4 {

class IPal4GameSystemBootstrapRuntime {
public:
    virtual ~IPal4GameSystemBootstrapRuntime() = default;
    virtual void SetGlobalFrameCallback(std::uint32_t global,
                                        std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t RenderSystem() = 0;
    virtual void SetRenderCallback(std::uint32_t system,
                                   std::uint32_t offset,
                                   std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    virtual void InitializeGameSystems(std::uint32_t game) = 0;
    [[nodiscard]] virtual std::uint32_t RegisterArenaEffectManager() = 0;
    virtual void SetUpdateHandler(std::uint32_t manager,
                                  std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t CameraSubsystem() = 0;
    [[nodiscard]] virtual std::uint32_t ActiveCameraObject(
        std::uint32_t subsystem) = 0;
    [[nodiscard]] virtual std::uint32_t CameraContainer(
        std::uint32_t active_camera) = 0;
    virtual void InitializeTexture(std::uint32_t texture_object,
                                   std::uint32_t container,
                                   std::uint32_t width,
                                   std::uint32_t height) = 0;
    [[nodiscard]] virtual std::int32_t InitializeStateAndUi(
        std::uint32_t game, std::uint32_t first,
        std::uint32_t second) = 0;
};

class Pal4GameSystemBootstrap final {
public:
    static constexpr std::uint32_t kGlobalFrameCallback = 0x94d56cu;
    static constexpr std::uint32_t kRenderFrame = 0x5c7f30u;
    static constexpr std::uint32_t kRenderFrameWithEffects = 0x5c8150u;
    static constexpr std::uint32_t kShutdown = 0x5c83a0u;
    static constexpr std::uint32_t kCheckAndShutdown = 0x5c83d0u;
    static constexpr std::uint32_t kCleanupResources = 0x5c8410u;
    static constexpr std::uint32_t kCleanupResourcesAlt = 0x5c8440u;

    // IDA 0x5c7e30.
    [[nodiscard]] static std::int32_t Initialize(
        IPal4GameSystemBootstrapRuntime& runtime);
};

}  // namespace pal4
