#pragma once

#include <cstdint>

namespace pal4 {

enum class GameArenaScriptSceneCall : std::uint8_t {
    AddClump,
    SetAnimationFlag,
    RenderCamera,
    ProcessClumpAtomics,
    GetCharacterProperty,
    InitializeProperties,
    CleanupCamera,
    SetStateFlag,
    CleanupObject,
    RemoveClump,
    GetObjectProperty,
    GetMovementAdapter,
    SetClumpRenderCallback,
    ProcessClumpAtomicsWrapper,
    GetPetInstance,
};

class IGameArenaScriptSceneRuntime {
public:
    virtual ~IGameArenaScriptSceneRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Call(
        GameArenaScriptSceneCall operation, std::uint32_t first = 0u,
        std::uint32_t second = 0u, std::uint32_t third = 0u) = 0;
};

class GameArenaScriptSceneRuntimeStub final
    : public IGameArenaScriptSceneRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Call(
        GameArenaScriptSceneCall, std::uint32_t,
        std::uint32_t, std::uint32_t) override { return 0u; }
};

class GameArenaScriptSceneCallbacks final {
public:
    // IDA 0x51c500, 0x51c590, 0x51c620 and 0x51c6d0.
    [[nodiscard]] static std::uint32_t AddObject(
        std::uint32_t world, std::uint32_t object,
        IGameArenaScriptSceneRuntime& runtime);
    [[nodiscard]] static std::uint32_t RemoveObject(
        std::uint32_t world, std::uint32_t object,
        IGameArenaScriptSceneRuntime& runtime);
    [[nodiscard]] static std::uint32_t AddPet(
        std::uint32_t world, IGameArenaScriptSceneRuntime& runtime);
    [[nodiscard]] static std::uint32_t RemovePet(
        std::uint32_t world, IGameArenaScriptSceneRuntime& runtime);
};

}  // namespace pal4
