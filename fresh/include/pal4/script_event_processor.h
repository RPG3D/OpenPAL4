#pragma once

#include <array>
#include <cstdint>

#include "pal4/string_buffer.h"

namespace pal4 {

struct ScriptEventProcessorState {
    std::array<std::uint32_t, 12> words{};
};
static_assert(sizeof(ScriptEventProcessorState) == 48u);

class IScriptEventProcessorRuntime {
public:
    virtual ~IScriptEventProcessorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeDocument(
        std::uint32_t address, const char* name,
        std::uint32_t option) = 0;
    virtual void CleanupDocument(std::uint32_t address) = 0;
    virtual void DestroyDocument(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void CleanupEventData(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class ScriptEventProcessorRuntimeStub final
    : public IScriptEventProcessorRuntime {
public:
    std::uint32_t Allocate(std::uint32_t bytes) override;
    std::uint32_t InitializeDocument(std::uint32_t address,
                                     const char* name,
                                     std::uint32_t option) override;
    void CleanupDocument(std::uint32_t address) override;
    void DestroyDocument(std::uint32_t address) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    void CleanupEventData(std::uint32_t address) override;
    std::uint32_t Free(std::uint32_t address) override;
};

class IScriptEventInsertRuntime {
public:
    virtual ~IScriptEventInsertRuntime() = default;
    [[nodiscard]] virtual bool AllocateText(
        StringBufferState& state, std::uint32_t length,
        std::uint8_t mode) = 0;
    virtual void CopyText(std::uint32_t destination, const char* source,
                          std::uint32_t length) = 0;
    virtual void SetTextSize(StringBufferState& state,
                             std::uint32_t length) = 0;
    virtual void AssignName(StringBufferState& destination,
                            const StringBufferState& source) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::int32_t CompareEventString(
        std::uint32_t event, std::uint32_t text) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeEventData(
        std::uint32_t address) = 0;
    virtual void AssignEventCString(std::uint32_t event,
                                    const char* source) = 0;
    virtual void InsertEventPointer(ScriptEventProcessorState& state,
                                    std::uint32_t position,
                                    std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class ScriptEventInsertRuntimeStub final : public IScriptEventInsertRuntime {
public:
    bool AllocateText(StringBufferState& state, std::uint32_t length,
                      std::uint8_t mode) override;
    void CopyText(std::uint32_t destination, const char* source,
                  std::uint32_t length) override;
    void SetTextSize(StringBufferState& state,
                     std::uint32_t length) override;
    void AssignName(StringBufferState& destination,
                    const StringBufferState& source) override;
    std::uint32_t EmptyStringAddress() override;
    std::int32_t CompareEventString(std::uint32_t event,
                                    std::uint32_t text) override;
    std::uint32_t Allocate(std::uint32_t bytes) override;
    std::uint32_t InitializeEventData(std::uint32_t address) override;
    void AssignEventCString(std::uint32_t event,
                            const char* source) override;
    void InsertEventPointer(ScriptEventProcessorState& state,
                            std::uint32_t position,
                            std::uint32_t event) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    void Free(std::uint32_t address) override;
};

class IScriptEventSchedulerRuntime {
public:
    virtual ~IScriptEventSchedulerRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t Random() = 0;
    [[nodiscard]] virtual std::uint32_t GetConditionDocument(
        std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t CallCondition(
        std::uint32_t callback, std::uint32_t document) = 0;
    virtual void InsertCandidate(ScriptEventProcessorState& state,
                                 std::uint32_t position,
                                 std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t DispatchEvent(
        std::uint32_t event, std::uint32_t task_context,
        std::uint32_t argument_two,
        std::uint32_t argument_three) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateContainer(
        std::uint32_t task_context, float elapsed) = 0;
};

class ScriptEventSchedulerRuntimeStub final
    : public IScriptEventSchedulerRuntime {
public:
    std::uint8_t ReadByte(std::uint32_t address) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    std::int32_t Random() override;
    std::uint32_t GetConditionDocument(std::uint32_t event) override;
    std::uint32_t CallCondition(std::uint32_t callback,
                                std::uint32_t document) override;
    void InsertCandidate(ScriptEventProcessorState& state,
                         std::uint32_t position,
                         std::uint32_t event) override;
    std::uint32_t DispatchEvent(std::uint32_t event,
                                std::uint32_t task_context,
                                std::uint32_t argument_two,
                                std::uint32_t argument_three) override;
    std::uint32_t UpdateContainer(std::uint32_t task_context,
                                  float elapsed) override;
};

class ScriptEventProcessor final {
public:
    [[nodiscard]] static std::uint8_t SetFlag24(
        ScriptEventProcessorState& state, std::uint8_t value);
    [[nodiscard]] static std::uint32_t SetFirstValue28(
        ScriptEventProcessorState& state, std::uint32_t value);
    [[nodiscard]] static std::uint32_t Initialize(
        ScriptEventProcessorState& state, std::uint32_t object_address,
        std::uint8_t incoming_stack_byte,
        IScriptEventProcessorRuntime& runtime);
    [[nodiscard]] static std::uint32_t ClearEvents(
        ScriptEventProcessorState& state,
        IScriptEventProcessorRuntime& runtime);
    [[nodiscard]] static std::uint32_t Cleanup(
        ScriptEventProcessorState& state,
        IScriptEventProcessorRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessAndAddEvent(
        ScriptEventProcessorState& state, const char* source,
        IScriptEventInsertRuntime& runtime);
    [[nodiscard]] static std::uint32_t FirstEvent(
        const ScriptEventProcessorState& state);
    [[nodiscard]] static std::uint32_t SelectRandomEvent(
        const ScriptEventProcessorState& state,
        IScriptEventSchedulerRuntime& runtime);
    [[nodiscard]] static std::uint32_t HandleEventProcessing(
        ScriptEventProcessorState& state, float elapsed,
        std::uint32_t task_context, std::uint32_t argument_two,
        std::uint32_t argument_three, std::uint32_t incoming_eax,
        IScriptEventSchedulerRuntime& runtime);
};

}  // namespace pal4
