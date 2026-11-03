#pragma once

#include "pal4/game_config_initializer.h"

#include <cstdint>

namespace pal4 {

class IGameSystemsRuntime : public IGameConfigLogger {
public:
    ~IGameSystemsRuntime() override = default;

    virtual void InitializeAllocator(std::uint32_t table_address) = 0;
    [[nodiscard]] virtual std::uint32_t EngineInit(
        std::uint32_t table_address, std::uint32_t flags,
        std::uint32_t resource_arena_size) = 0;
    [[nodiscard]] virtual std::uint32_t AttachPlugins() = 0;
    [[nodiscard]] virtual std::uint32_t BindInputCallback(
        std::uint32_t selector, std::uint32_t callback_address) = 0;
    [[nodiscard]] virtual std::uint32_t OpenEngine(
        std::uint32_t window) = 0;
    virtual void ShutdownEngine() = 0;
    [[nodiscard]] virtual std::uint32_t SelectDevice() = 0;
    virtual void SetRefreshRate(std::uint32_t rate) = 0;
    // Original 0x760E60 controls D3D9 dialog-box mode, including creation flags.
    // It must be configured before EngineStart; it is not a fullscreen switch.
    virtual void EnableDialogBoxMode(std::uint32_t enabled) = 0;
    [[nodiscard]] virtual std::uint32_t StartEngine() = 0;
    virtual void CloseEngine() = 0;
    virtual void StopEngine() = 0;
    // Borrowed D3DCAPS9 pointer; startup stores it at state+0x55c.
    [[nodiscard]] virtual std::uint32_t GetD3D9Caps() = 0;
    [[nodiscard]] virtual std::uint32_t GetCurrentD3DDevice() = 0;
    [[nodiscard]] virtual std::uint32_t InitializeImageHandlers() = 0;
    [[nodiscard]] virtual std::uint32_t OpenCharset() = 0;
    [[nodiscard]] virtual std::uint32_t OpenFileSystemManager(
        std::uint32_t sentinel) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeVirtualFileSystem() = 0;
    [[nodiscard]] virtual std::uint32_t InitializeDirectInput(
        std::uint32_t instance, std::uint32_t window,
        std::uint32_t flags) = 0;
    virtual void SetInputParameters(std::uint32_t first,
                                    std::uint32_t second,
                                    std::uint32_t width,
                                    std::uint32_t height) = 0;
    virtual void SetInputScalar(std::uint32_t float_bits) = 0;
};

class GameSystemsInitializer final {
public:
    static constexpr std::uint32_t kAllocatorTable = 0x8e3d98u;
    // RwEngineInit third argument: a 4 MiB resource arena, not an EXE address.
    static constexpr std::uint32_t kResourceArenaSize = 0x400000u;
    static constexpr std::uint32_t kKeyboardCallback = 0x409630u;
    static constexpr std::uint32_t kMouseCallback = 0x409670u;

    GameSystemsInitializer(GameConfigWords& config, GameRenderState& state)
        : config_(config), state_(state) {}

    [[nodiscard]] std::uint32_t Initialize(IGameSystemsRuntime& runtime);

private:
    GameConfigWords& config_;
    GameRenderState& state_;
};

}  // namespace pal4
