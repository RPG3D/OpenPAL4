#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct PlayerActionWaitState {
    std::uint32_t vtable_address{};
    std::array<std::byte, 8> base{};
    std::uint32_t phase{};
    std::array<std::byte, 16> name_buffer{};
    std::uint32_t completed_loops{};
    std::int32_t execute_flag{};
    std::uint32_t actor{};
};
static_assert(sizeof(PlayerActionWaitState) == 0x2cu);
static_assert(offsetof(PlayerActionWaitState, completed_loops) == 0x20u);

struct PlayerEndActionWaitState {
    std::uint32_t vtable_address{};
    std::array<std::byte, 28> base{};
    std::uint32_t actor{};
};
static_assert(sizeof(PlayerEndActionWaitState) == 0x24u);

class IScriptPlayerActionRuntime {
public:
    virtual ~IScriptPlayerActionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentPlayer(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t NpcManager() = 0;
    [[nodiscard]] virtual std::uint32_t NpcByName(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::int32_t SetAction(
        std::uint32_t actor, std::string_view action) = 0;
    virtual void SetFlag(std::uint32_t actor, std::uint32_t flag) = 0;
    virtual void ClearFlag(std::uint32_t actor, std::uint32_t flag) = 0;
    [[nodiscard]] virtual bool GetFlag(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::string_view ActionName(std::uint32_t actor) = 0;
    virtual void ApplyEquipmentVisuals(std::uint32_t actor) = 0;
    virtual void ClearWeaponEffects(std::uint32_t actor) = 0;
    virtual void UpdateEffectList(std::uint32_t actor) = 0;
    virtual void UpdateEffectList2(std::uint32_t actor) = 0;
    virtual void UpdateEffectList3(std::uint32_t actor) = 0;
    virtual void UpdateEffectList4(std::uint32_t actor) = 0;
    virtual void CleanupWeaponEffects(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::string_view FormatActorName(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual PlayerActionWaitState* AllocateAction(
        std::uint32_t size) = 0;
    [[nodiscard]] virtual PlayerEndActionWaitState* AllocateEnd(
        std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void ConstructAction(PlayerActionWaitState& state,
                                 std::uint8_t mode,
                                 std::uint32_t queue) = 0;
    virtual void ConstructEnd(PlayerEndActionWaitState& state,
                              std::uint32_t mode,
                              std::uint32_t queue) = 0;
    virtual void AssignName(PlayerActionWaitState& state,
                            std::string_view name) = 0;
    virtual void FlushAction(PlayerActionWaitState& state) = 0;
    virtual void FlushEnd(PlayerEndActionWaitState& state) = 0;
    virtual void AddAction(std::uint32_t queue,
                           PlayerActionWaitState* state) = 0;
    [[nodiscard]] virtual std::uint32_t AddEnd(
        std::uint32_t queue, PlayerEndActionWaitState* state) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupName() = 0;
    virtual void CloseAction(PlayerActionWaitState& state) = 0;
    virtual void CloseEnd(PlayerEndActionWaitState& state) = 0;
};

class ScriptPlayerAction final {
public:
    static constexpr std::uint32_t kActionVtable = 0x84952cu;
    static constexpr std::uint32_t kEndVtable = 0x849538u;
    static constexpr std::uint32_t kQueueOffset = 0x54u;

    explicit ScriptPlayerAction(IScriptPlayerActionRuntime& runtime)
        : runtime_(runtime) {}

    // IDA 0x5e0100, 0x5e0370, 0x5e0440, 0x5e04f0,
    // 0x5e0560, 0x5e0570, 0x5e07f0, 0x5e3340, 0x5e3540,
    // and 0x5e5bf0..0x5e6100.
    [[nodiscard]] std::uint32_t StartIndexed(std::int32_t index,
                                             std::string_view action,
                                             std::int32_t execute_flag,
                                             std::uint8_t wait_mode);
    void UpdateAction(PlayerActionWaitState& state);
    // IDA5E6E30, the action handler virtual +8 cleanup.
    void ResetAction(PlayerActionWaitState& state);
    [[nodiscard]] std::uint32_t EndIndexed(std::int32_t index);
    void UpdateEnd(PlayerEndActionWaitState& state);
    static void NoOp();
    [[nodiscard]] std::uint32_t StartCurrent(std::string_view action,
                                             std::int32_t execute_flag,
                                             std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t EndCurrent();
    [[nodiscard]] std::uint32_t StartNpc(
        std::string_view npc_name, std::string_view action,
        std::int32_t execute_flag, std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t EndNpc(std::string_view npc_name);
    [[nodiscard]] std::uint32_t StartRepeatIndexed(
        std::int32_t index, std::string_view action);
    [[nodiscard]] std::uint32_t EndRepeatIndexed(std::int32_t index);
    [[nodiscard]] std::uint32_t StartRepeatNpc(
        std::string_view npc_name, std::string_view action);
    [[nodiscard]] std::uint32_t EndRepeatNpc(std::string_view npc_name);

private:
    [[nodiscard]] std::uint32_t Start(
        std::uint32_t actor, std::string_view action,
        std::int32_t execute_flag, std::uint8_t wait_mode,
        bool npc, std::string_view npc_name, bool repeat = false);
    [[nodiscard]] std::uint32_t End(std::uint32_t actor);
    [[nodiscard]] std::uint32_t EndRepeat(std::uint32_t actor);

    IScriptPlayerActionRuntime& runtime_;
};

}  // namespace pal4
