#pragma once

#include <cstdint>

namespace pal4 {

class IEventPropertyBindingRuntime {
public:
    virtual ~IEventPropertyBindingRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadValue(
        std::uint32_t source, std::uint32_t key,
        std::uint32_t vtable_offset) = 0;
    [[nodiscard]] virtual double ReadDouble(
        std::uint32_t source, std::uint32_t key,
        std::uint32_t vtable_offset) = 0;
    [[nodiscard]] virtual std::uint32_t NameRecord(
        std::uint32_t source, std::uint32_t vtable_offset) = 0;
    [[nodiscard]] virtual std::uint8_t WriteFloat(
        std::uint32_t target, std::uint32_t name,
        std::uint32_t float_bits) = 0;
    [[nodiscard]] virtual std::uint8_t WriteInteger(
        std::uint32_t target, std::uint32_t name,
        std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t WriteString(
        std::uint32_t target, std::uint32_t name,
        std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ConfigString(
        std::uint32_t target, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t ParseFloatBits(
        std::uint32_t text) = 0;
    [[nodiscard]] virtual std::uint32_t ParseInteger(
        std::uint32_t text) = 0;
    [[nodiscard]] virtual std::uint32_t SetSourceValue(
        std::uint32_t source, std::uint32_t key,
        std::uint32_t value, std::uint32_t vtable_offset) = 0;
};

class EventPropertyBindingRuntimeStub final
    : public IEventPropertyBindingRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint32_t ReadValue(std::uint32_t source, std::uint32_t key,
                            std::uint32_t vtable_offset) override;
    double ReadDouble(std::uint32_t source, std::uint32_t key,
                      std::uint32_t vtable_offset) override;
    std::uint32_t NameRecord(std::uint32_t source,
                             std::uint32_t vtable_offset) override;
    std::uint8_t WriteFloat(std::uint32_t target, std::uint32_t name,
                            std::uint32_t float_bits) override;
    std::uint8_t WriteInteger(std::uint32_t target, std::uint32_t name,
                              std::uint32_t value) override;
    std::uint8_t WriteString(std::uint32_t target, std::uint32_t name,
                             std::uint32_t value) override;
    std::uint32_t ConfigString(std::uint32_t target,
                               std::uint32_t name) override;
    std::uint32_t ParseFloatBits(std::uint32_t text) override;
    std::uint32_t ParseInteger(std::uint32_t text) override;
    std::uint32_t SetSourceValue(std::uint32_t source, std::uint32_t key,
                                 std::uint32_t value,
                                 std::uint32_t vtable_offset) override;
};

class EventPropertyBinding final {
public:
    // IDA 0x5efbe0 / 0x5efc20 / 0x5efc70 / 0x5efcb0 / 0x5efd00.
    [[nodiscard]] static std::uint8_t WriteFloat(
        std::uint32_t wrapper, std::uint32_t source,
        IEventPropertyBindingRuntime& runtime);
    [[nodiscard]] static std::uint8_t WriteString(
        std::uint32_t wrapper, std::uint32_t source,
        IEventPropertyBindingRuntime& runtime);
    [[nodiscard]] static std::uint8_t WritePointerString(
        std::uint32_t wrapper, std::uint32_t source,
        IEventPropertyBindingRuntime& runtime);
    [[nodiscard]] static std::uint8_t WriteByte(
        std::uint32_t wrapper, std::uint32_t source,
        IEventPropertyBindingRuntime& runtime);
    [[nodiscard]] static std::uint8_t WriteInteger(
        std::uint32_t wrapper, std::uint32_t source,
        IEventPropertyBindingRuntime& runtime);
    // IDA 0x5efd40.
    [[nodiscard]] static std::uint32_t SetFloatFromConfig(
        std::uint32_t wrapper, std::uint32_t source,
        IEventPropertyBindingRuntime& runtime);
    // IDA 0x5f0000 / 0x5f0060.
    [[nodiscard]] static std::uint32_t SetBoolFromConfig(
        std::uint32_t wrapper, std::uint32_t source,
        IEventPropertyBindingRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetIntegerFromConfig(
        std::uint32_t wrapper, std::uint32_t source,
        IEventPropertyBindingRuntime& runtime);
};

}  // namespace pal4
