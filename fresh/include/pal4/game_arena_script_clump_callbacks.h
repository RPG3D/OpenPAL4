#pragma once

#include <cstdint>
#include <initializer_list>

namespace pal4 {

enum class GameArenaScriptClumpCall : std::uint8_t {
    GetCameraSubsystem,
    AddCameraClump,
    RemoveCameraClump,
    GetMovementAdapter,
    GetResourceProperty,
    SetClumpProperties,
    RestoreClumpRenderCallback,
    CleanupObject,
    AddClump,
    Virtual72,
    ProcessClumpAtomics,
    ProcessAtomics,
    ProcessAtomicsFinal,
    Virtual140,
    Virtual156,
    Virtual76,
    RemoveClump,
    ResetAtomicCallbacks,
    ResetMaterialCallbacks,
    ProcessClumpAtomicsWrapper,
};

class IGameArenaScriptClumpRuntime {
public:
    virtual ~IGameArenaScriptClumpRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Call(
        GameArenaScriptClumpCall operation,
        std::initializer_list<std::uint32_t> arguments) = 0;
};

class GameArenaScriptClumpRuntimeStub final
    : public IGameArenaScriptClumpRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Call(
        GameArenaScriptClumpCall,
        std::initializer_list<std::uint32_t>) override { return 0u; }
};

class GameArenaScriptClumpCallbacks final {
public:
    // IDA 0x51c760/0x51c810 and 0x51c890/0x51c950.
    [[nodiscard]] static std::uint32_t ActivateClump(
        std::uint32_t world, std::uint32_t object,
        IGameArenaScriptClumpRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeactivateClump(
        std::uint32_t world, std::uint32_t object,
        IGameArenaScriptClumpRuntime& runtime);
    [[nodiscard]] static std::uint32_t AddAnimatedObject(
        std::uint32_t world, std::uint32_t object,
        IGameArenaScriptClumpRuntime& runtime);
    [[nodiscard]] static std::uint32_t RemoveAnimatedObject(
        std::uint32_t world, std::uint32_t object,
        IGameArenaScriptClumpRuntime& runtime);
};

}  // namespace pal4
