#pragma once

#include <cstdint>

namespace pal4 {

class IUtilitiesTextRecordRuntime {
public:
    virtual ~IUtilitiesTextRecordRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class UtilitiesTextRecordCleanup final {
public:
    // IDA 0x506140: release text triplets at +24 and +8.
    [[nodiscard]] static std::uint32_t Release8And24(
        std::uint32_t record, IUtilitiesTextRecordRuntime& runtime);
    // IDA 0x5061b0: release text triplets at +40, +24 and +8.
    [[nodiscard]] static std::uint32_t Release8And24And40(
        std::uint32_t record, IUtilitiesTextRecordRuntime& runtime);
    // IDA 0x506250: release text triplets at +28 and +12.
    [[nodiscard]] static std::uint32_t Release12And28(
        std::uint32_t record, IUtilitiesTextRecordRuntime& runtime);
    // IDA 0x5062c0: release text triplets at +28 and +8.
    [[nodiscard]] static std::uint32_t Release8And28(
        std::uint32_t record, IUtilitiesTextRecordRuntime& runtime);
};

}  // namespace pal4
