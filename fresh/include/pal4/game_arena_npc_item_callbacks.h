#pragma once

#include <cstdint>
#include <initializer_list>

namespace pal4 {

enum class GameArenaNpcItemCall : std::uint8_t {
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
    ResetAtomicCallbacks,
    ResetMaterialCallbacks,
    ProcessClumpAtomicsWrapper,
    RemoveClump,
};

class IGameArenaNpcItemRuntime {
public:
    virtual ~IGameArenaNpcItemRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Call(
        GameArenaNpcItemCall operation,
        std::initializer_list<std::uint32_t> arguments) = 0;
};

class GameArenaNpcItemRuntimeStub final : public IGameArenaNpcItemRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Call(
        GameArenaNpcItemCall,
        std::initializer_list<std::uint32_t>) override { return 0u; }
};

class GameArenaNpcItemCallbacks final {
public:
    // IDA 0x51bb00/0x51bbd0 and 0x51bc60/0x51bd30.
    [[nodiscard]] static std::uint32_t EnterNpc(
        std::uint32_t npc, std::uint32_t world,
        IGameArenaNpcItemRuntime& runtime);
    [[nodiscard]] static std::uint32_t LeaveNpc(
        std::uint32_t npc, std::uint32_t world,
        IGameArenaNpcItemRuntime& runtime);
    [[nodiscard]] static std::uint32_t EnterItem(
        std::uint32_t item, std::uint32_t world,
        IGameArenaNpcItemRuntime& runtime);
    [[nodiscard]] static std::uint32_t LeaveItem(
        std::uint32_t item, std::uint32_t world,
        IGameArenaNpcItemRuntime& runtime);
};

}  // namespace pal4
