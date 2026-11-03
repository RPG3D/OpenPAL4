#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGobObstaclePhysicsRuntime;
class IGobObstacleConfigRuntime;
class IGobObstacleEventRuntime;

class IGobObstacleRuntime {
public:
    virtual ~IGobObstacleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void ConstructMachine(std::uint32_t object) = 0;
    virtual void DestructMachine(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupMachineResources(
        std::uint32_t object) = 0;
    virtual void InitializeObstacleHelper(std::uint32_t helper) = 0;
    virtual void DestructObstacleHelper(std::uint32_t helper) = 0;
    virtual void InitializeStringRecord(std::uint32_t record) = 0;
    virtual void ReleaseStringRecord(std::uint32_t record) = 0;
    virtual void InitializeEffectHandle(std::uint32_t handle) = 0;
    virtual void ResetEffectHandle(std::uint32_t handle) = 0;
    [[nodiscard]] virtual bool DialogNeedsUpdate(std::uint32_t handle) = 0;
    virtual void UpdateDialogContent(std::uint32_t handle) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class GobObstacle final {
public:
    static constexpr std::uint32_t kSize = 0x334u;
    static constexpr std::uint32_t kVtable = 0x8491a8u;
    static constexpr std::uint32_t kStateVtable = 0x84919cu;
    static constexpr std::uint32_t kStateOffset = 0x1dcu;
    static constexpr std::uint32_t kHelperOffset = 0x2ccu;
    static constexpr std::uint32_t kEffectHandleOffset = 0x32cu;

    [[nodiscard]] static std::uint32_t Create(IGobObstacleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IGobObstacleRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetObstacleRuntimeHelper(
        std::uint32_t object);
    [[nodiscard]] static std::uint32_t TypeKey();
    static void Destruct(std::uint32_t object, IGobObstacleRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateDialogAndCleanup(
        std::uint32_t object, IGobObstacleRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateMachineAndApplyActorForce(
        std::uint32_t object, float elapsed,
        IGobObstaclePhysicsRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateLaunchEffectOrDialog(
        std::uint32_t object, float elapsed,
        IGobObstaclePhysicsRuntime& runtime);
    static void ApplyEffectTypeToActor(
        std::uint32_t object, IGobObstaclePhysicsRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeSceneAndConfig(
        std::uint32_t object, std::uint32_t first, std::string_view path,
        std::uint32_t fourth, std::uint32_t fifth,
        IGobObstacleConfigRuntime& runtime);
    [[nodiscard]] static std::uint32_t RunScriptAndProcessEvent(
        std::uint32_t object, std::uint32_t event,
        IGobObstacleEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetTriggeredRuntime(
        std::uint32_t object, IGobObstacleEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetConfigNode(
        std::uint32_t object, IGobObstacleConfigRuntime& runtime);
    [[nodiscard]] static std::uint32_t ReadConfigAndResetLaunchRuntime(
        std::uint32_t object, IGobObstacleConfigRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateStateSnapshot(
        std::uint32_t object, IGobObstacleEventRuntime& runtime);
};

}  // namespace pal4
