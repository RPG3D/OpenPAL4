#pragma once

#include "pal4/actor_control_database.h"

#include <array>
#include <cstdint>

namespace pal4 {

using ActorControlWorldPoint = std::array<float, 3>;
using ActorControlScreenPoint = std::array<float, 2>;

class IActorControlDisplayResetRuntime {
public:
    virtual ~IActorControlDisplayResetRuntime() = default;
    [[nodiscard]] virtual ActorControlWorldPoint ActorPosition(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual float ActorHeight(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t CameraSubsystem() = 0;
    [[nodiscard]] virtual std::uint32_t ActiveCamera(
        std::uint32_t subsystem) = 0;
    [[nodiscard]] virtual ActorControlScreenPoint Project(
        std::uint32_t camera, const ActorControlWorldPoint& position) = 0;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    virtual void ProcessUiState(std::uint32_t manager,
                                std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t MonsterManager() = 0;
    virtual void ClearMonsterFlags(std::uint32_t manager) = 0;
    [[nodiscard]] virtual ActorControlScreenPoint ConvertCoordinates(
        std::uint32_t manager, const ActorControlScreenPoint& projection,
        const ActorControlScreenPoint& mixed) = 0;
    [[nodiscard]] virtual std::uint8_t ProcessUiEvents(
        std::uint32_t manager, std::uint32_t mode,
        const ActorControlScreenPoint& position) = 0;
};

class IActorControlDisplayRuntime : public IActorControlDisplayResetRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ProgressBar(
        std::uint32_t manager) = 0;
    virtual void SetProgressVisible(std::uint32_t bar, bool visible) = 0;
    [[nodiscard]] virtual std::uint32_t GameArenaManager() = 0;
    [[nodiscard]] virtual std::uint8_t RemoveClump(
        std::uint32_t arena, std::uint32_t clump) = 0;
};

class ActorControlDisplayRuntimeStub : public IActorControlDisplayRuntime {
public:
    ActorControlWorldPoint ActorPosition(std::uint32_t) override;
    float ActorHeight(std::uint32_t) override;
    std::uint32_t CameraSubsystem() override;
    std::uint32_t ActiveCamera(std::uint32_t) override;
    ActorControlScreenPoint Project(
        std::uint32_t, const ActorControlWorldPoint&) override;
    std::uint32_t UiFrameManager() override;
    void ProcessUiState(std::uint32_t, std::uint32_t) override;
    std::uint32_t MonsterManager() override;
    void ClearMonsterFlags(std::uint32_t) override;
    ActorControlScreenPoint ConvertCoordinates(
        std::uint32_t, const ActorControlScreenPoint&,
        const ActorControlScreenPoint&) override;
    std::uint8_t ProcessUiEvents(
        std::uint32_t, std::uint32_t,
        const ActorControlScreenPoint&) override;
    std::uint32_t ProgressBar(std::uint32_t) override;
    void SetProgressVisible(std::uint32_t, bool) override;
    std::uint32_t GameArenaManager() override;
    std::uint8_t RemoveClump(std::uint32_t, std::uint32_t) override;
};

}  // namespace pal4
