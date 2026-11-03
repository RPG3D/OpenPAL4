#pragma once

#include <cstdint>

namespace pal4 {

enum class GameArenaPlayerCall : std::uint8_t {
    AddClump,
    SetAnimationFlag,
    RenderCamera,
    ProcessClumpAtomics,
    GetCharacterProperty,
    InitializeProperties,
    CleanupCamera,
    GetObjectProperty,
    SetStateFlag,
    CleanupObject,
    RemoveClump,
    GetMovementAdapter,
    SetClumpRenderCallback,
    ProcessClumpAtomicsWrapper,
};

class IGameArenaPlayerRuntime {
public:
    virtual ~IGameArenaPlayerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Call(
        GameArenaPlayerCall operation, std::uint32_t first = 0u,
        std::uint32_t second = 0u, std::uint32_t third = 0u) = 0;
};

class GameArenaPlayerRuntimeStub final : public IGameArenaPlayerRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Call(GameArenaPlayerCall,
                                      std::uint32_t, std::uint32_t,
                                      std::uint32_t) override { return 0u; }
};

class GameArenaPlayerCallbacks final {
public:
    // IDA 0x51b760 and 0x51b7f0.
    [[nodiscard]] static std::uint32_t Enter(
        std::uint32_t player, std::uint32_t world,
        IGameArenaPlayerRuntime& runtime);
    [[nodiscard]] static std::uint32_t Leave(
        std::uint32_t player, std::uint32_t world,
        IGameArenaPlayerRuntime& runtime);
};

}  // namespace pal4
