#pragma once

#include "pal4/string_buffer.h"

#include <cstdint>

namespace pal4 {

class IEventPropertyValueRuntime {
public:
    virtual ~IEventPropertyValueRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void IncrementTempOff() = 0;
    virtual void DecrementTempOff() = 0;
    [[nodiscard]] virtual std::uint32_t FloatBits(
        std::uint32_t source) = 0;
    [[nodiscard]] virtual std::int32_t IntegerValue(
        std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t StringRecord(
        std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t SetValue(
        std::uint32_t property, std::uint32_t key,
        std::uint32_t value) = 0;
    virtual void InitializeString(StringBufferState& temporary,
                                  std::uint32_t text) = 0;
    virtual void SetEnumValue(std::uint32_t property, std::uint32_t key,
                              const StringBufferState& temporary) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupString(
        StringBufferState& temporary) = 0;
};

class EventPropertyValueRuntimeStub final : public IEventPropertyValueRuntime {
public:
    std::uint32_t Allocate(std::uint32_t bytes) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    void IncrementTempOff() override;
    void DecrementTempOff() override;
    std::uint32_t FloatBits(std::uint32_t source) override;
    std::int32_t IntegerValue(std::uint32_t source) override;
    std::uint32_t StringRecord(std::uint32_t source) override;
    std::uint32_t SetValue(std::uint32_t property, std::uint32_t key,
                           std::uint32_t value) override;
    void InitializeString(StringBufferState& temporary,
                          std::uint32_t text) override;
    void SetEnumValue(std::uint32_t property, std::uint32_t key,
                      const StringBufferState& temporary) override;
    std::uint32_t CleanupString(StringBufferState& temporary) override;
};

class EventPropertyValue final {
public:
    // IDA 0x5ef550 / 0x5ef620 / 0x5ef6d0 / 0x5ef780 /
    // 0x5ef880 / 0x5ef930.
    [[nodiscard]] static std::uint32_t CreateFloat(
        std::uint32_t owner, std::uint32_t operand,
        IEventPropertyValueRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateInt(
        std::uint32_t owner, std::uint32_t operand,
        IEventPropertyValueRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateString(
        std::uint32_t owner, std::uint32_t operand,
        IEventPropertyValueRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateEnum(
        std::uint32_t owner, std::uint32_t operand,
        IEventPropertyValueRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateBool(
        std::uint32_t owner, std::uint32_t operand,
        IEventPropertyValueRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateUnknown(
        std::uint32_t owner, std::uint32_t operand,
        IEventPropertyValueRuntime& runtime);
    // IDA 0x5ef5d0 / 0x5ef6a0 / 0x5ef750 / 0x5ef800 / 0x5ef900.
    [[nodiscard]] static std::uint32_t SetFloatFromFloat(
        std::uint32_t owner, std::uint32_t key, std::uint32_t source,
        IEventPropertyValueRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetIntFromInt(
        std::uint32_t owner, std::uint32_t key, std::uint32_t source,
        IEventPropertyValueRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetStringFromString(
        std::uint32_t owner, std::uint32_t key, std::uint32_t source,
        IEventPropertyValueRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetEnumFromString(
        std::uint32_t owner, std::uint32_t key, std::uint32_t source,
        IEventPropertyValueRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetBoolFromInt(
        std::uint32_t owner, std::uint32_t key, std::uint32_t source,
        IEventPropertyValueRuntime& runtime);
    // IDA 0x5ef600 / 0x5ef610.
    [[nodiscard]] static std::uint32_t GetType();
    static void Cleanup(std::uint32_t property,
                        IEventPropertyValueRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t owner, std::uint32_t operand,
        std::uint32_t vtable, IEventPropertyValueRuntime& runtime);
};

}  // namespace pal4
