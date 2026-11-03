#pragma once

#include <cstdint>

namespace pal4 {

class ICombatDataStructureRuntime {
public:
    virtual ~ICombatDataStructureRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    virtual void CleanupEventData(std::uint32_t payload) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyVirtual(
        std::uint32_t object, std::uint32_t deleting_flag) = 0;
};

class CombatDataStructure final {
public:
    // IDA 0x576bf0.
    [[nodiscard]] static std::uint32_t CleanupEventCollection(
        std::uint32_t collection, ICombatDataStructureRuntime& runtime);
    // IDA 0x576c50.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t first, std::uint32_t second,
        ICombatDataStructureRuntime& runtime);
    // IDA 0x576c80.
    [[nodiscard]] static std::uint32_t DestroyRange(
        std::uint32_t begin, std::uint32_t end,
        ICombatDataStructureRuntime& runtime);
};

}  // namespace pal4
