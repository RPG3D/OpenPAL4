#pragma once

#include <cstdint>
#include <initializer_list>

namespace pal4 {

enum class GameArenaScriptCharacterCall : std::uint8_t {
    AddClump,
    GetMovementAdapter,
    SetClumpProperties,
    SetAnimationFlag,
    RenderCamera,
    Virtual72,
    ProcessClumpAtomics,
    ProcessAtomics,
    ProcessAtomicsFinal,
    GetCharacterProperty,
    InitializeProperties,
    CleanupCamera,
    GetObjectProperty,
    SetStateFlag,
    CleanupObject,
    Virtual68,
    Virtual76,
    SetClumpRenderCallback,
    RemoveClump,
    ResetAtomicCallbacks,
    ResetMaterialCallbacks,
    ProcessClumpAtomicsWrapper,
};

class IGameArenaScriptCharacterRuntime {
public:
    virtual ~IGameArenaScriptCharacterRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Call(
        GameArenaScriptCharacterCall operation,
        std::initializer_list<std::uint32_t> arguments) = 0;
};

class GameArenaScriptCharacterRuntimeStub final
    : public IGameArenaScriptCharacterRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Call(
        GameArenaScriptCharacterCall,
        std::initializer_list<std::uint32_t>) override { return 0u; }
};

class GameArenaScriptCharacterCallbacks final {
public:
    // IDA 0x51ca00, 0x51caa0 and 0x51cb70.
    [[nodiscard]] static std::uint32_t Enter(
        std::uint32_t world, std::uint32_t character,
        IGameArenaScriptCharacterRuntime& runtime);
    [[nodiscard]] static std::uint32_t LeaveWithVirtual68(
        std::uint32_t world, std::uint32_t character,
        IGameArenaScriptCharacterRuntime& runtime);
    [[nodiscard]] static std::uint32_t Leave(
        std::uint32_t world, std::uint32_t character,
        IGameArenaScriptCharacterRuntime& runtime);
};

}  // namespace pal4
