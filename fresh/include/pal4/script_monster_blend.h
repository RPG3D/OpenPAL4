#pragma once

#include "pal4/script_object_blend.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

// Native monster, arena, animation, RenderWare and script operations.
class IScriptMonsterBlendRuntime {
public:
    virtual ~IScriptMonsterBlendRuntime() = default;
    [[nodiscard]] virtual std::uint32_t MonsterManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindMonster(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual ObjectBlendWaitState* AllocateWait(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void ConstructWait(ObjectBlendWaitState& state,
                               std::uint8_t mode,
                               std::uint32_t queue) = 0;
    virtual void FlushWait(ObjectBlendWaitState& state) = 0;
    [[nodiscard]] virtual std::uint32_t AddHandler(
        std::uint32_t queue, ObjectBlendWaitState* state) = 0;
    virtual void SetMaterialFlags(std::uint32_t monster,
                                  std::uint32_t flags) = 0;
    virtual void LeaveCharacter(std::uint32_t monster) = 0;
    virtual void EnterCharacter(std::uint32_t monster) = 0;
    virtual void SetStateFlag(std::uint32_t monster,
                              std::uint32_t flag) = 0;
    [[nodiscard]] virtual std::string_view MonsterName(
        std::uint32_t monster) = 0;
    [[nodiscard]] virtual std::uint32_t FindMonsterInfo(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual bool SetAction(std::uint32_t monster,
                                         std::string_view action,
                                         float blend) = 0;
    virtual void ReplaceComponent(std::uint32_t monster,
                                  std::uint32_t info,
                                  std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint8_t InterpolateAlpha(
        float start, float end, float ratio) = 0;
    virtual void CloseWait(ObjectBlendWaitState& state) = 0;
};

// Monster fade lifecycle; the fade-in update slot points to 0x5e7620.
class ScriptMonsterBlend final {
public:
    static constexpr std::uint32_t kOutVtable = 0x849630u;
    static constexpr std::uint32_t kInVtable = 0x84963cu;
    static constexpr std::uint32_t kQueueOffset = 0x54u;

    explicit ScriptMonsterBlend(IScriptMonsterBlendRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5e76c0: queue fade-out for a named monster.
    [[nodiscard]] std::uint32_t BlendOut(std::string_view monster_name,
                                         float duration,
                                         std::uint8_t wait_mode);
    // 0x5e77b0: update fade-out and leave arena at completion.
    void UpdateOut(ObjectBlendWaitState& state, float delta);
    // 0x5e7860: restore flags, leave arena and close the handler.
    void FinishOut(ObjectBlendWaitState& state);
    // 0x5e78b0: re-enter monster, restore idle action and queue fade-in.
    [[nodiscard]] std::uint32_t BlendIn(std::string_view monster_name,
                                        float duration,
                                        std::uint8_t wait_mode);
    // Monster vtable 0x84963c+4 shares IDA 0x5e7620 fade-in update.
    void UpdateIn(ObjectBlendWaitState& state, float delta);
    // 0x5e3300: fade-in cancellation restores color without leaving the arena.
    void FinishIn(ObjectBlendWaitState& state);

private:
    [[nodiscard]] std::uint32_t Create(std::string_view monster_name,
                                       float duration,
                                       std::uint8_t wait_mode, bool fade_in);

    IScriptMonsterBlendRuntime& runtime_;
};

}  // namespace pal4
