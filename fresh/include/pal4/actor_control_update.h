#pragma once

#include "pal4/actor_control_database.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

using ActorControlVector = std::array<float, 3>;

struct ActorControlPlayerPrefix {
    std::array<std::uint32_t, 93> words{};
};
static_assert(sizeof(ActorControlPlayerPrefix) == 372u);

struct ActorControlInput {
    std::int32_t held{};
    std::uint32_t action{};
    std::uint32_t released{};
};

struct ActorControlGroundResult {
    bool found{};
    float height{};
};

struct ActorControlMovementBounds {
    std::array<float, 4> upper{};
    std::array<float, 4> lower{};
};

struct ActorControlCollisionResult {
    ActorControlVector contact{};
    ActorControlVector response{};
};
static_assert(sizeof(ActorControlCollisionResult) == 24u);

class IActorControlUpdateRuntime {
public:
    virtual ~IActorControlUpdateRuntime() = default;
    [[nodiscard]] virtual ActorControlVector PlayerForward(
        const ActorControlPlayerPrefix& player) = 0;
    [[nodiscard]] virtual double PlayerProperty(
        const ActorControlPlayerPrefix& player) = 0;
    [[nodiscard]] virtual bool InteractionBlocked(
        ActorControlDatabasePrefix& state) = 0;
    [[nodiscard]] virtual bool ProgressVisible() = 0;
    [[nodiscard]] virtual double ProgressValue() = 0;
    virtual void SetProgressVisible(bool visible) = 0;
    [[nodiscard]] virtual ActorControlInput ReadInput() = 0;
    virtual void Normalize(ActorControlVector& value) = 0;
    [[nodiscard]] virtual ActorControlGroundResult FindGround(
        const ActorControlVector& position, float radius, float height) = 0;
    [[nodiscard]] virtual ActorControlGroundResult CheckGround(
        const ActorControlVector& position, float radius, float height) = 0;
    virtual void SetResourcePosition(ActorControlDatabasePrefix& state,
                                     const ActorControlVector& position,
                                     std::uint32_t mode) = 0;
    virtual void MoveResource(ActorControlDatabasePrefix& state,
                              const ActorControlVector& velocity,
                              std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t HAnimFrameId(std::uint32_t clump) = 0;
    virtual void AddClump(std::uint32_t clump) = 0;
    virtual void RemoveClump(std::uint32_t clump) = 0;
    virtual void ProcessClumpAtomics(std::uint32_t clump) = 0;
    virtual void ConfigureTransition(ActorControlDatabasePrefix& state,
                                     const ActorControlVector& start,
                                     const ActorControlVector& finish,
                                     float range) = 0;
    virtual void PlayerCommand(ActorControlPlayerPrefix& player,
                               std::string_view command) = 0;
    virtual void SnapPlayerPosition(ActorControlPlayerPrefix& player,
                                    const ActorControlVector& position,
                                    std::uint32_t mode) = 0;
    virtual void SetAnimationFlag(ActorControlPlayerPrefix& player,
                                  std::uint32_t flag) = 0;
    virtual void ProcessUiEvents(std::uint32_t mode) = 0;
    virtual void SetMonsterFlags() = 0;
    virtual void PlayMusic(std::string_view path, std::uint32_t mode) = 0;
    [[nodiscard]] virtual ActorControlVector ScaleMovement(float seconds) = 0;
    [[nodiscard]] virtual float MovementHeight() = 0;
    // IDA422DA4 uses the game's approximate planar range helper. The later
    // fall-distance gate uses public RenderWare length through VectorLength.
    [[nodiscard]] virtual double PlanarRangeMagnitude(
        const ActorControlVector& value);
    [[nodiscard]] virtual float VectorLength(const ActorControlVector& value) = 0;
    virtual void ProcessMovement(ActorControlPlayerPrefix& player,
                                 std::uint32_t first_mode,
                                 std::uint32_t collision_mode,
                                 std::uint32_t third_mode,
                                 const ActorControlMovementBounds& bounds,
                                 ActorControlVector& movement) = 0;
    virtual void AdjustPlayerPosition(ActorControlVector& position,
                                      float radius, float range,
                                      std::uint32_t mode) = 0;
    virtual void SetPlayerPosition(ActorControlPlayerPrefix& player,
                                   const ActorControlVector& position) = 0;
    virtual void UpdateMinimap(const ActorControlPlayerPrefix& player) = 0;
    [[nodiscard]] virtual ActorControlVector CameraPosition() = 0;
    virtual void SetAudioPosition(const ActorControlVector& position) = 0;
    [[nodiscard]] virtual std::uint32_t CollisionObject() = 0;
    [[nodiscard]] virtual bool SweepCollision(
        const ActorControlVector& start, const ActorControlVector& finish,
        std::uint32_t object, ActorControlCollisionResult& result) = 0;
    virtual void ResolvePlayerCollision(ActorControlPlayerPrefix& player,
                                        const ActorControlVector& hit,
                                        std::uint32_t mode) = 0;
    [[nodiscard]] virtual bool NameNodeNeedsUpdate(
        ActorControlDatabasePrefix& state) = 0;
    virtual void HandleNameEvent(ActorControlDatabasePrefix& state) = 0;
    virtual void HandleNameEventWithContext(
        ActorControlDatabasePrefix& state, std::uint32_t context,
        std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::string_view FloorSound() = 0;
    virtual void PlayFloorSound(std::string_view sound,
                                const ActorControlVector& position,
                                std::uint32_t mode, float radius,
                                float volume) = 0;
    [[nodiscard]] virtual std::array<std::uint32_t, 2> CreateEffect(
        std::string_view name, const ActorControlVector& position,
        std::uint32_t first, std::uint32_t second) = 0;
    virtual void ReleaseTemporaryEffect() = 0;
    virtual void SetEffectVisibility(ActorControlDatabasePrefix& state,
                                     const ActorControlPlayerPrefix& player) = 0;
    virtual void InitializeScript() = 0;
    [[nodiscard]] virtual std::string_view ArenaScriptPath() = 0;
    [[nodiscard]] virtual bool ScriptCached(std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t CreateInterpreter(
        std::string_view base_path, std::string_view path,
        std::string_view script_name) = 0;
    [[nodiscard]] virtual bool InterpreterRaisedException(
        std::uint32_t interpreter) = 0;
    virtual void HandleScriptException(std::uint32_t interpreter) = 0;
    virtual void ReleaseInterpreter(std::uint32_t interpreter) = 0;
    virtual void ShowScriptError() = 0;
    virtual void CleanupScripts() = 0;
};

class ActorControlUpdateRuntimeStub : public IActorControlUpdateRuntime {
public:
    ActorControlVector PlayerForward(const ActorControlPlayerPrefix&) override;
    double PlayerProperty(const ActorControlPlayerPrefix&) override;
    bool InteractionBlocked(ActorControlDatabasePrefix&) override;
    bool ProgressVisible() override;
    double ProgressValue() override;
    void SetProgressVisible(bool) override;
    ActorControlInput ReadInput() override;
    void Normalize(ActorControlVector&) override;
    ActorControlGroundResult FindGround(const ActorControlVector&, float,
                                        float) override;
    ActorControlGroundResult CheckGround(const ActorControlVector&, float,
                                         float) override;
    void SetResourcePosition(ActorControlDatabasePrefix&,
                             const ActorControlVector&,
                             std::uint32_t) override;
    void MoveResource(ActorControlDatabasePrefix&,
                      const ActorControlVector&, std::uint32_t) override;
    std::uint32_t HAnimFrameId(std::uint32_t) override;
    void AddClump(std::uint32_t) override;
    void RemoveClump(std::uint32_t) override;
    void ProcessClumpAtomics(std::uint32_t) override;
    void ConfigureTransition(ActorControlDatabasePrefix&,
                             const ActorControlVector&,
                             const ActorControlVector&, float) override;
    void PlayerCommand(ActorControlPlayerPrefix&, std::string_view) override;
    void SnapPlayerPosition(ActorControlPlayerPrefix&,
                            const ActorControlVector&,
                            std::uint32_t) override;
    void SetAnimationFlag(ActorControlPlayerPrefix&, std::uint32_t) override;
    void ProcessUiEvents(std::uint32_t) override;
    void SetMonsterFlags() override;
    void PlayMusic(std::string_view, std::uint32_t) override;
    ActorControlVector ScaleMovement(float) override;
    float MovementHeight() override;
    float VectorLength(const ActorControlVector&) override;
    void ProcessMovement(ActorControlPlayerPrefix&, std::uint32_t,
                         std::uint32_t, std::uint32_t,
                         const ActorControlMovementBounds&,
                         ActorControlVector&) override;
    void AdjustPlayerPosition(ActorControlVector&, float, float,
                              std::uint32_t) override;
    void SetPlayerPosition(ActorControlPlayerPrefix&,
                           const ActorControlVector&) override;
    void UpdateMinimap(const ActorControlPlayerPrefix&) override;
    ActorControlVector CameraPosition() override;
    void SetAudioPosition(const ActorControlVector&) override;
    std::uint32_t CollisionObject() override;
    bool SweepCollision(const ActorControlVector&, const ActorControlVector&,
                        std::uint32_t, ActorControlCollisionResult&) override;
    void ResolvePlayerCollision(ActorControlPlayerPrefix&,
                                const ActorControlVector&,
                                std::uint32_t) override;
    bool NameNodeNeedsUpdate(ActorControlDatabasePrefix&) override;
    void HandleNameEvent(ActorControlDatabasePrefix&) override;
    void HandleNameEventWithContext(ActorControlDatabasePrefix&,
                                    std::uint32_t, std::uint32_t) override;
    std::string_view FloorSound() override;
    void PlayFloorSound(std::string_view,
                        const ActorControlVector&, std::uint32_t,
                        float, float) override;
    std::array<std::uint32_t, 2> CreateEffect(
        std::string_view, const ActorControlVector&,
        std::uint32_t, std::uint32_t) override;
    void ReleaseTemporaryEffect() override;
    void SetEffectVisibility(ActorControlDatabasePrefix&,
                             const ActorControlPlayerPrefix&) override;
    void InitializeScript() override;
    std::string_view ArenaScriptPath() override;
    bool ScriptCached(std::string_view) override;
    std::uint32_t CreateInterpreter(std::string_view, std::string_view,
                                    std::string_view) override;
    bool InterpreterRaisedException(std::uint32_t) override;
    void HandleScriptException(std::uint32_t) override;
    void ReleaseInterpreter(std::uint32_t) override;
    void ShowScriptError() override;
    void CleanupScripts() override;
};

}  // namespace pal4
