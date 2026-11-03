#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IEffectSystemTaskVariantRuntime {
public:
    virtual ~IEffectSystemTaskVariantRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
    virtual void ResetMessageString(std::uint32_t object) = 0;
    virtual void ResetSlowMotion(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupCommon(
        std::uint32_t object) = 0;
};

class EffectSystemTaskVariants final {
public:
    static constexpr std::uint32_t kMessageVtable = 0x84b2d8u;
    static constexpr std::uint32_t kSlowMotionVtable = 0x84b318u;

    // IDA 0x641220/0x641350: virtual task type names.
    [[nodiscard]] static constexpr std::string_view MessageTypeName() {
        return "MessageTask";
    }
    [[nodiscard]] static constexpr std::string_view SlowMotionTypeName() {
        return "SlowMotionTask";
    }

    // IDA 0x641250/0x641380: derived cleanup followed by the common base.
    [[nodiscard]] static std::uint32_t CleanupMessage(
        std::uint32_t object, IEffectSystemTaskVariantRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupSlowMotion(
        std::uint32_t object, IEffectSystemTaskVariantRuntime& runtime);
};

}  // namespace pal4
