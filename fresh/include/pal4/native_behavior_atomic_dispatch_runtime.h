#pragma once

#include "pal4/behavior_atomic_dispatcher.h"

namespace pal4 {
class NativeEventDocumentRuntime;
class NativeScriptObjectRuntime;

class INativeBehaviorTaskFactory {
public:
    virtual ~INativeBehaviorTaskFactory() = default;
    [[nodiscard]] virtual std::uint32_t Create(std::uint32_t tree,
        std::int32_t class_id, std::uint32_t container, const char* name,
        const BehaviorAtomicWaitOptions& options) = 0;
    virtual void Bind(std::uint32_t task, std::uint32_t owner,
                       std::uint32_t record) = 0;
};

// Native document and task-container services for the existing dispatcher.
// Concrete task construction and owner binding are mandatory dependencies.
class NativeBehaviorAtomicDispatchRuntime final : public IBehaviorAtomicDispatchRuntime {
public:
    NativeBehaviorAtomicDispatchRuntime(NativeEventDocumentRuntime& documents,
        NativeScriptObjectRuntime& tasks, INativeBehaviorTaskFactory& factory)
        : documents_(documents), tasks_(tasks), factory_(factory) {}
    void CleanupCollection(std::uint32_t context) override;
    std::uint32_t Rand() override;
    [[noreturn]] void DivideByZero() override;
    std::uint32_t FindAttribute(std::uint32_t record, const char* name) override;
    std::int32_t ToInt(std::uint32_t attribute, std::int32_t fallback) override;
    float ToFloat(std::uint32_t attribute, float fallback) override;
    std::uint32_t ProcessEvent(std::uint32_t context, std::int32_t class_id,
        std::uint32_t task_container, const char* name,
        const BehaviorAtomicWaitOptions& options) override;
    void NotifyTask(std::uint32_t task, std::uint32_t owner,
                      std::uint32_t record) override;
    void MarkProcessed(std::uint32_t context) override;
private:
    NativeEventDocumentRuntime& documents_;
    NativeScriptObjectRuntime& tasks_;
    INativeBehaviorTaskFactory& factory_;
};
}  // namespace pal4
