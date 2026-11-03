#pragma once

#include <cstdint>

namespace pal4 {

class IGameObjectFrameLifecycleRuntime {
public:
    virtual ~IGameObjectFrameLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void InitEffectSystem(std::uint32_t object) = 0;
    virtual void ResetManager(std::uint32_t manager) = 0;
    virtual void CleanupTree(std::uint32_t holder,
                             std::uint32_t root) = 0;
    virtual void GetFirstElement(std::uint32_t holder,
                                 std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t NextNode(std::uint32_t node) = 0;
    virtual void RemoveNode(std::uint32_t holder,
                            std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyEffectSystem(
        std::uint32_t object) = 0;
    virtual void DisableEffectSystem(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual std::uint32_t CurrentCameraSlot(
        std::uint32_t combat) = 0;
    virtual void EnableEffectSystem(std::uint32_t object,
                                    std::uint32_t camera,
                                    std::uint32_t game_property) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeManagers(
        std::uint32_t manager) = 0;
};

class GameObjectFrameLifecycle final {
public:
    static constexpr std::uint32_t kSharedTreeHead = 0x8e4110u;
    static constexpr std::uint32_t kSharedTreeCount = 0x8f6070u;
    static constexpr std::uint32_t kVtable = 0x8452e8u;
    static constexpr std::uint32_t kFrameCallback = 0x53dcf0u;

    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IGameObjectFrameLifecycleRuntime& runtime);
    // Native host owns the handler catalog. No VC6 tree/head is created.
    [[nodiscard]] static std::uint32_t ConstructHostManaged(
        std::uint32_t object, IGameObjectFrameLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyHostManaged(
        std::uint32_t object, IGameObjectFrameLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IGameObjectFrameLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint8_t Initialize(
        std::uint32_t object, IGameObjectFrameLifecycleRuntime& runtime);
    static void DeinitializeHelper(
        std::uint32_t object, IGameObjectFrameLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetUpdateHandler(
        std::uint32_t object, std::uint32_t callback,
        IGameObjectFrameLifecycleRuntime& runtime);
    static void DestroyManagerTree(
        std::uint32_t manager, IGameObjectFrameLifecycleRuntime& runtime);
};

}  // namespace pal4
