#pragma once

#include "pal4/dialog_object.h"
#include <string>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativeScriptObjectRuntime;

// Payload dispatch must be bound by the effect owner; no EXE vtable calls.
class INativeDialogTaskPayloads {
public:
    virtual ~INativeDialogTaskPayloads() = default;
    virtual bool EffectsEnabled() = 0;
    virtual bool EffectIsActive(std::uint32_t effect) = 0;
    virtual std::uint32_t CreateTask(std::uint32_t xml) = 0;
    virtual void BindDialogObject(std::uint32_t task, std::uint32_t object) = 0;
    virtual void PrepareTask(std::uint32_t task) = 0;
    virtual void StartTask(std::uint32_t task) = 0;
};

// Borrows the same native document, allocator and task owner as playback/reset.
class NativeDialogTaskRuntime final : public IDialogObjectTaskRuntime {
public:
    NativeDialogTaskRuntime(NativeBaseObjectMemory&, NativeEventDocumentRuntime&,
        NativeScriptObjectRuntime&, INativeDialogTaskPayloads&);
    std::uint32_t ReadWord(std::uint32_t, std::uint32_t) override;
    void WriteWord(std::uint32_t, std::uint32_t, std::uint32_t) override;
    bool EffectsEnabled() override;
    bool EffectIsActive(std::uint32_t) override;
    std::optional<int> IntAttribute(std::uint32_t, std::string_view, bool) override;
    std::optional<float> FloatAttribute(std::uint32_t, std::string_view) override;
    // Returned view remains valid until the next StringAttribute call.
    std::optional<std::string_view> StringAttribute(std::uint32_t, std::string_view) override;
    std::uint32_t CreateTask(std::uint32_t) override;
    void AddTaskAndInitialize(std::uint32_t, std::uint32_t, std::string_view,
        const DialogTaskTiming&) override;
    void BindDialogObject(std::uint32_t, std::uint32_t) override;
    void PrepareTask(std::uint32_t) override;
    void StartTask(std::uint32_t) override;
private:
    NativeBaseObjectMemory& memory_;
    NativeEventDocumentRuntime& documents_;
    NativeScriptObjectRuntime& tasks_;
    INativeDialogTaskPayloads& payloads_;
    std::string attribute_text_;
};
}  // namespace pal4
