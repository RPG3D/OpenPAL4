#pragma once

#include <cstdint>
#include <string>

namespace pal4 {

class IGobMachineTriggerRuntime {
public:
    virtual ~IGobMachineTriggerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t MovementAdapterRoot() = 0;
    virtual void SetClumpRenderCallback(std::uint32_t adapter,
                                        std::uint32_t clump) = 0;
    [[nodiscard]] virtual std::uint32_t ForwardMovementSlot84(
        std::uint32_t object) = 0;
    virtual void ClearTaskCollection(std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::uint32_t CheckAndConsumeInventoryConditions(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t StartTaskRun(
        std::uint32_t object) = 0;
    virtual void MarkEventProcessed(std::uint32_t event) = 0;
    [[nodiscard]] virtual std::string ArenaName() = 0;
    [[nodiscard]] virtual std::string SceneName() = 0;
    [[nodiscard]] virtual std::string ObjectName(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Crc32(
        const std::string& value) = 0;
    virtual void DispatchGameEvent(std::uint32_t crc) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseEventNameBuffer() = 0;
};

class GobMachineTrigger final {
public:
    [[nodiscard]] static std::uint32_t InitializeMovementAdapterAndCall(
        std::uint32_t object, IGobMachineTriggerRuntime& runtime);
    [[nodiscard]] static std::uint32_t TryStartTaskRun(
        std::uint32_t object, IGobMachineTriggerRuntime& runtime);
};

}  // namespace pal4
