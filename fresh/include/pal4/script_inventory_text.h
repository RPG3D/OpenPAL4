#pragma once

#include "pal4/string_buffer.h"

#include <cstdint>

namespace pal4 {

struct ScriptInventoryTextState {
    std::uint8_t first_byte{};
    std::uint8_t guard{};
    StringBufferState text{};
};

struct InventoryDescription {
    std::uint32_t data{};
    std::uint32_t length{};
};

class IScriptInventoryTextRuntime {
public:
    virtual ~IScriptInventoryTextRuntime() = default;
    virtual void SetErrorFlag(ScriptInventoryTextState& state,
                              std::uint8_t flag) = 0;
    virtual void RegisterAtExit(std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::int32_t InvokeValue(
        std::uint32_t callback_object, std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t FindEntry(
        std::int32_t value, std::uint32_t category) = 0;
    [[nodiscard]] virtual InventoryDescription GetDescription(
        std::uint32_t entry) = 0;
    [[nodiscard]] virtual bool AllocateText(StringBufferState& text,
                                            std::uint32_t length,
                                            std::uint8_t mode) = 0;
    virtual void CopyLiteral(std::uint32_t destination, const char* source,
                             std::uint32_t length) = 0;
    virtual void CopyDescription(std::uint32_t destination,
                                 std::uint32_t source,
                                 std::uint32_t length) = 0;
    virtual void SetTextSize(StringBufferState& text,
                             std::uint32_t length) = 0;
    [[noreturn]] virtual void ThrowLength() = 0;
};

class ScriptInventoryTextRuntimeStub final
    : public IScriptInventoryTextRuntime {
public:
    void SetErrorFlag(ScriptInventoryTextState& state,
                      std::uint8_t flag) override;
    void RegisterAtExit(std::uint32_t callback) override;
    std::int32_t InvokeValue(std::uint32_t callback_object,
                             std::uint32_t argument) override;
    std::uint32_t FindEntry(std::int32_t value,
                            std::uint32_t category) override;
    InventoryDescription GetDescription(std::uint32_t entry) override;
    bool AllocateText(StringBufferState& text, std::uint32_t length,
                      std::uint8_t mode) override;
    void CopyLiteral(std::uint32_t destination, const char* source,
                     std::uint32_t length) override;
    void CopyDescription(std::uint32_t destination, std::uint32_t source,
                         std::uint32_t length) override;
    void SetTextSize(StringBufferState& text,
                     std::uint32_t length) override;
    [[noreturn]] void ThrowLength() override;
};

class IScriptEventValueRuntime {
public:
    virtual ~IScriptEventValueRuntime() = default;
    [[nodiscard]] virtual const char* ReadCString(
        std::uint32_t argument_record) = 0;
    [[nodiscard]] virtual std::int32_t InvokeParsedValue(
        std::uint32_t callback_object, std::int32_t value) = 0;
};

class ScriptEventValueRuntimeStub final : public IScriptEventValueRuntime {
public:
    const char* ReadCString(std::uint32_t argument_record) override;
    std::int32_t InvokeParsedValue(std::uint32_t callback_object,
                                   std::int32_t value) override;
};

class IScriptInventoryTextCleanupRuntime {
public:
    virtual ~IScriptInventoryTextCleanupRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class ScriptInventoryTextCleanupRuntimeStub final
    : public IScriptInventoryTextCleanupRuntime {
public:
    std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override;
    void Free(std::uint32_t address) override;
};

class ScriptInventoryText final {
public:
    static constexpr std::uint32_t kBufferAddress = 0x8e4210u;
    static constexpr std::uint32_t kCleanupCallback = 0x42ebf0u;

    [[nodiscard]] static std::uint32_t Format(
        ScriptInventoryTextState& state, std::uint32_t callback_object,
        std::uint32_t argument, IScriptInventoryTextRuntime& runtime);
    [[nodiscard]] static std::int32_t ForwardParsedValue(
        std::uint32_t callback_object, std::uint32_t argument_record,
        IScriptEventValueRuntime& runtime);
    static void CleanupGlobal(ScriptInventoryTextState& state,
                              IScriptInventoryTextCleanupRuntime& runtime);
};

}  // namespace pal4
