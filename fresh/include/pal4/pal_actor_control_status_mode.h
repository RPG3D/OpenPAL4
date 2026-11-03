#pragma once

#include "pal4/pal_actor_control_invincibility.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class IActorControlStatusModeRuntime {
public:
    virtual ~IActorControlStatusModeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UiFrameManager();
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, std::string_view name);
    [[nodiscard]] virtual bool IsFrameVisible(std::uint32_t frame);
    virtual void ProcessUiEvent(std::uint32_t queue, std::uint32_t event,
                                const std::array<std::uint32_t, 2>& payload);
    virtual void StartPlayerStatus(PalActorControlState& state,
                                   float first, float second, float value);
    virtual void SetAnimationFlag(std::uint32_t animation,
                                  std::uint32_t value);
    [[nodiscard]] virtual std::uint32_t MonsterManager();
    virtual void ClearMonsterFlags(std::uint32_t manager);
    [[nodiscard]] virtual std::uint32_t ProcessMonsters(
        std::uint32_t manager, long double context);
    [[nodiscard]] virtual bool DialogNeedsUpdate(std::uint32_t* handle);
    virtual void UpdateDialog(std::uint32_t* handle);
    [[nodiscard]] virtual std::uint32_t PlayerManager();
    [[nodiscard]] virtual std::uint32_t FindPlayer(
        std::uint32_t manager, std::uint32_t index);
    virtual void ClearStatusHandles(PalActorControlState& state);
    [[nodiscard]] virtual bool PositionInRange(std::uint32_t frame);
    virtual void BeginMovementStatus(PalActorControlState& state,
                                     float duration);
    virtual void ApplyStatusIfPlayerExists(PalActorControlState& state,
                                           float amount);
    [[nodiscard]] virtual ActorControlEffectCreation CreateEffect(
        std::string_view name, std::uint32_t position,
        std::uint32_t first_flag, std::uint32_t second_flag);
    virtual void ResetTemporary(ActorControlEffectHandle& temporary);
    [[nodiscard]] virtual std::uint32_t AudioManager();
    [[nodiscard]] virtual std::uint32_t PlayAudio(
        std::uint32_t manager, const std::uint32_t* mode,
        std::string_view path, std::uint32_t flag);
    virtual void PlayAudioScratch(std::uint32_t manager,
                                  std::string_view path, std::uint32_t flag);
};

class PalActorControlStatusMode final {
public:
    // IDA 0x426b70.
    [[nodiscard]] static std::uint32_t Activate(
        PalActorControlState& state, long double monster_context,
        std::uint32_t mode, IActorControlStatusModeRuntime& runtime);
    // IDA 0x426d10. Returns one only while the matching dialog is visible.
    [[nodiscard]] static std::uint32_t Poll(
        PalActorControlState& state,
        IActorControlStatusModeRuntime& runtime);
};

}  // namespace pal4
