#pragma once

#include "pal4/string_buffer.h"

#include <cstdint>

namespace pal4 {

struct EventScriptSearchState {
    std::uint32_t tag{};
    std::uint32_t first{};
    std::uint32_t second{};
    std::uint32_t third{};
};
static_assert(sizeof(EventScriptSearchState) == 16u);

struct EventScriptTextState {
    std::uint32_t value{};
    std::uint32_t data{};
};
static_assert(sizeof(EventScriptTextState) == 8u);

class IEventPropertyScriptRuntime {
public:
    virtual ~IEventPropertyScriptRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t NameRecord(
        std::uint32_t source, std::uint32_t vtable_offset) = 0;
    [[nodiscard]] virtual std::uint32_t StringLength(
        std::uint32_t text) = 0;
    virtual void SetErrorFlag(EventScriptSearchState& search,
                              std::uint8_t flag) = 0;
    virtual void SetSearchText(EventScriptSearchState& search,
                                std::uint32_t text,
                                std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t FindNode(
        std::uint32_t tree, const EventScriptSearchState& search) = 0;
    virtual void ConvertAttribute(std::uint32_t attribute,
                                  EventScriptTextState& output,
                                  std::uint32_t empty_text) = 0;
    virtual void SetRecord(std::uint32_t source, std::uint32_t key,
                           const EventScriptTextState& text,
                           std::uint32_t vtable_offset) = 0;
    virtual void InitializeString(StringBufferState& output,
                                  std::uint32_t text) = 0;
    virtual void CleanupString(StringBufferState& output) = 0;
    virtual void SetString(std::uint32_t source, std::uint32_t key,
                           const StringBufferState& text,
                           std::uint32_t vtable_offset) = 0;
};

class EventPropertyScriptRuntimeStub final
    : public IEventPropertyScriptRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    std::uint32_t Free(std::uint32_t address) override;
    std::uint32_t NameRecord(std::uint32_t source,
                             std::uint32_t vtable_offset) override;
    std::uint32_t StringLength(std::uint32_t text) override;
    void SetErrorFlag(EventScriptSearchState& search,
                      std::uint8_t flag) override;
    void SetSearchText(EventScriptSearchState& search,
                        std::uint32_t text, std::uint32_t length) override;
    std::uint32_t FindNode(
        std::uint32_t tree,
        const EventScriptSearchState& search) override;
    void ConvertAttribute(std::uint32_t attribute,
                          EventScriptTextState& output,
                          std::uint32_t empty_text) override;
    void SetRecord(std::uint32_t source, std::uint32_t key,
                   const EventScriptTextState& text,
                   std::uint32_t vtable_offset) override;
    void InitializeString(StringBufferState& output,
                          std::uint32_t text) override;
    void CleanupString(StringBufferState& output) override;
    void SetString(std::uint32_t source, std::uint32_t key,
                   const StringBufferState& text,
                   std::uint32_t vtable_offset) override;
};

class EventPropertyScript final {
public:
    // IDA 0x5efda0 / 0x5efeb0.
    [[nodiscard]] static std::uint32_t SetFromScript(
        std::uint32_t wrapper, std::uint32_t source,
        IEventPropertyScriptRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetStringFromScript(
        std::uint32_t wrapper, std::uint32_t source,
        IEventPropertyScriptRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t Find(
        std::uint32_t wrapper, std::uint32_t source,
        std::uint32_t& owner,
        IEventPropertyScriptRuntime& runtime);
    [[nodiscard]] static std::uint32_t ReleaseText(
        const EventScriptTextState& text,
        IEventPropertyScriptRuntime& runtime);
};

}  // namespace pal4
