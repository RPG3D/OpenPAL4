#pragma once

#include <cstdint>

namespace pal4 {

class ICombatEventObjectRecordRuntime {
public:
    virtual ~ICombatEventObjectRecordRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ApplyModeZero(
        std::uint32_t event, std::uint32_t target,
        std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ApplyModeOne(
        std::uint32_t event, std::uint32_t target,
        std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Query(
        std::uint32_t event, std::uint32_t value_address,
        std::uint32_t argument) = 0;
};

class CombatEventObjectRecord final {
public:
    // IDA 0x581500.
    [[nodiscard]] static std::uint32_t Process(
        std::uint32_t record, std::uint32_t target,
        ICombatEventObjectRecordRuntime& runtime);
    // IDA 0x581540.
    [[nodiscard]] static std::uint8_t Query(
        std::uint32_t record, std::uint32_t argument,
        ICombatEventObjectRecordRuntime& runtime);
};

}  // namespace pal4
