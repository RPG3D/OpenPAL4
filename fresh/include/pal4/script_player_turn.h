#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct PlayerTurnWaitState {
    std::uint32_t vtable_address{};
    std::array<std::byte, 8> base{};
    std::uint32_t phase{};
    std::array<std::byte, 16> name_buffer{};
    std::uint32_t actor{};
    float delta{};
    float start_yaw{};
    float speed{};
    float duration{};
    float elapsed{};
};
static_assert(sizeof(PlayerTurnWaitState) == 0x38u);
static_assert(offsetof(PlayerTurnWaitState, actor) == 0x20u);

struct PlayerTurnVector {
    float x{};
    float y{};
    float z{};
};

class IScriptPlayerTurnRuntime {
public:
    virtual ~IScriptPlayerTurnRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentPlayer(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::int32_t CurrentPlayerIndex(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectManager() = 0;
    [[nodiscard]] virtual std::uint32_t ObjectByName(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t NpcManager() = 0;
    [[nodiscard]] virtual std::uint32_t NpcByName(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t NpcInfoByName(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual float BaselineYaw(std::uint32_t npc_info) = 0;
    [[nodiscard]] virtual PlayerTurnVector ReadPosition(
        std::uint32_t actor) = 0;
    virtual void ClampFacingVector(PlayerTurnVector& vector) = 0;
    [[nodiscard]] virtual float FacingAngleDegrees(
        const PlayerTurnVector& vector) = 0;
    [[nodiscard]] virtual PlayerTurnWaitState* Allocate(
        std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void Construct(PlayerTurnWaitState& state,
                           std::uint8_t wait_mode,
                           std::uint32_t queue) = 0;
    virtual void Flush(PlayerTurnWaitState& state) = 0;
    [[nodiscard]] virtual bool AllocateText(PlayerTurnWaitState& state,
                                            std::uint32_t size) = 0;
    virtual void AssignName(PlayerTurnWaitState& state,
                            std::string_view name) = 0;
    [[nodiscard]] virtual float ReadYaw(std::uint32_t actor) = 0;
    [[nodiscard]] virtual float ReadX(std::uint32_t actor) = 0;
    [[nodiscard]] virtual float ReadZ(std::uint32_t actor) = 0;
    virtual void SetRotation(std::uint32_t actor, float x,
                             float yaw, float z, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::int32_t SetAction(
        std::uint32_t actor, std::string_view action) = 0;
    [[nodiscard]] virtual std::int32_t SetTimedAction(
        std::uint32_t actor, std::string_view action,
        float blend) = 0;
    virtual void CleanupWeaponEffects(std::uint32_t actor) = 0;
    virtual void Close(PlayerTurnWaitState& state) = 0;
    virtual void AddHandler(std::uint32_t queue,
                            PlayerTurnWaitState* state) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupName() = 0;
};

class ScriptPlayerTurn final {
public:
    static constexpr std::uint32_t kVtable = 0x849570u;
    static constexpr std::uint32_t kQueueOffset = 0x54u;
    static constexpr float kSpeed = 21.0f;
    static constexpr float kBlend = 0.2f;

    explicit ScriptPlayerTurn(IScriptPlayerTurnRuntime& runtime)
        : runtime_(runtime) {}

    // IDA 0x5e13c0..0x5e19f0, 0x5e3ae0..0x5e4050,
    // 0x5e4be0..0x5e51a0 and 0x5e5950.
    [[nodiscard]] std::uint32_t Start(std::int32_t index,
                                      float target_yaw,
                                      std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t FacePlayer(std::int32_t source_index,
                                           std::int32_t target_index,
                                           std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t FaceNpc(std::int32_t source_index,
                                        std::string_view npc_name,
                                        std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t StartNpc(std::string_view npc_name,
                                         float target_yaw,
                                         std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t FaceNpcToNpc(
        std::string_view source_name, std::string_view target_name,
        std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t FaceNpcToPlayer(
        std::string_view source_name, std::int32_t target_index,
        std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t FacePlayerToCurrent(
        std::int32_t source_index, std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t FaceCurrentToNpc(
        std::string_view target_name, std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t FaceNpcToCurrent(
        std::string_view source_name, std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t ResetNpcDirection(
        std::string_view npc_name);
    // IDA 0x5e8310: current player faces a named game object.
    [[nodiscard]] std::uint32_t FaceCurrentToObject(
        std::string_view object_name, std::uint8_t wait_mode);
    void Update(PlayerTurnWaitState& state, double elapsed);
    void Finish(PlayerTurnWaitState& state);

private:
    [[nodiscard]] std::uint32_t StartActor(
        std::uint32_t actor, std::string_view name,
        float target_yaw, std::uint8_t wait_mode);
    [[nodiscard]] std::uint32_t FaceActor(
        std::uint32_t source, std::uint32_t target,
        std::string_view name, std::uint8_t wait_mode);
    void RestoreIdle(PlayerTurnWaitState& state);
    IScriptPlayerTurnRuntime& runtime_;
};

}  // namespace pal4
