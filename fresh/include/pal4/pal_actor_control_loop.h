#pragma once

#include "pal4/pal_actor_control.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IPalActorControlLoopRuntime {
public:
    virtual ~IPalActorControlLoopRuntime() = default;
    virtual void SetCursor(std::string_view name);
    virtual void ProcessInput(PalActorControlState& state);
    virtual void ProcessInputAndClearVector(PalActorControlState& state);
    virtual void InitializeInputState();
    virtual void ProcessMonsters();
    virtual void ClearMonsterFlags();
    virtual void SetActorAnimationFlag(std::uint32_t actor,
                                       std::uint32_t value);
    virtual bool DialogNeedsUpdate(std::uint32_t* dialog);
    virtual void UpdateDialog(std::uint32_t* dialog);
    virtual std::uint8_t UpdateDialogResult(std::uint32_t* dialog);
    virtual void UpdateGameStateConditional(PalActorControlState& state,
                                            std::uint32_t value);
    virtual void UpdateGameStateAndUi(PalActorControlState& state,
                                      std::uint32_t actor);
    virtual void UpdateDialogStatus(PalActorControlState& state,
                                    std::uint32_t value);
    virtual std::uint32_t GetPetManager();
    virtual bool PetActive(std::uint32_t pet);
    virtual std::uint32_t GetArenaManager();
    virtual void ScriptPetOnRemove(std::uint32_t arena);
    virtual void PetVirtualRemove(std::uint32_t pet);
    virtual std::uint32_t ResetPet(std::uint32_t pet);
    virtual void PetVirtualLoad(std::uint32_t pet,
                                std::string_view group,
                                std::string_view id,
                                std::uint32_t enabled);
    virtual void InitializePetData(std::uint32_t pet,
                                   std::uint32_t actor);
    virtual void SetPetStateFlag(std::uint32_t pet,
                                 std::uint32_t value);
    virtual std::uint32_t ScriptPetOnAdd(std::uint32_t arena);
    virtual void ResetPlayerDatabase(std::uint32_t* database,
                                     std::uint32_t actor,
                                     std::uint32_t event_context);
};

class PalActorControlLoop final {
public:
    // IDA 0x423ac0: the sole stack argument selects the two update paths.
    [[nodiscard]] static std::uint32_t ResetAndSync(
        PalActorControlState& state, bool clear_monster_flags,
        IPalActorControlLoopRuntime& runtime);
    [[nodiscard]] static std::uint8_t ClearActor(
        PalActorControlState& state,
        IPalActorControlLoopRuntime& runtime);
    // IDA 0x4264a0.
    static void ResetPlayerToJumpStart(
        PalActorControlState& state, std::uint32_t event_context,
        IPalActorControlLoopRuntime& runtime);
};

}  // namespace pal4
