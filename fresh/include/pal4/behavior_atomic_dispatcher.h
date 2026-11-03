#pragma once

#include "pal4/script_event_data.h"

#include <cstdint>

namespace pal4 {

struct BehaviorAtomicWaitOptions {
    std::int32_t mode = 3;
    float wait_time = 0.0f;
    float expect_time = 0.0f;
};
static_assert(sizeof(BehaviorAtomicWaitOptions) == 12u);

class IBehaviorAtomicDispatchRuntime {
public:
    virtual ~IBehaviorAtomicDispatchRuntime() = default;
    virtual void CleanupCollection(std::uint32_t context) = 0;
    [[nodiscard]] virtual std::uint32_t Rand() = 0;
    [[noreturn]] virtual void DivideByZero() = 0;
    [[nodiscard]] virtual std::uint32_t FindAttribute(
        std::uint32_t record, const char* name) = 0;
    [[nodiscard]] virtual std::int32_t ToInt(
        std::uint32_t attribute, std::int32_t fallback) = 0;
    [[nodiscard]] virtual float ToFloat(
        std::uint32_t attribute, float fallback) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessEvent(
        std::uint32_t context, std::int32_t class_id,
        std::uint32_t task_container, const char* atomic_name,
        const BehaviorAtomicWaitOptions& options) = 0;
    virtual void NotifyTask(std::uint32_t task,
                            std::uint32_t notification_argument,
                            std::uint32_t record) = 0;
    virtual void MarkProcessed(std::uint32_t context) = 0;
};

class BehaviorAtomicDispatchRuntimeStub final
    : public IBehaviorAtomicDispatchRuntime {
public:
    void CleanupCollection(std::uint32_t context) override;
    std::uint32_t Rand() override;
    [[noreturn]] void DivideByZero() override;
    std::uint32_t FindAttribute(
        std::uint32_t record, const char* name) override;
    std::int32_t ToInt(
        std::uint32_t attribute, std::int32_t fallback) override;
    float ToFloat(std::uint32_t attribute, float fallback) override;
    std::uint32_t ProcessEvent(
        std::uint32_t context, std::int32_t class_id,
        std::uint32_t task_container, const char* atomic_name,
        const BehaviorAtomicWaitOptions& options) override;
    void NotifyTask(std::uint32_t task,
                    std::uint32_t notification_argument,
                    std::uint32_t record) override;
    void MarkProcessed(std::uint32_t context) override;
};

class BehaviorAtomicDispatcher final {
public:
    [[nodiscard]] static std::uint8_t Dispatch(
        ScriptEventDataState& state, std::uint32_t context,
        std::uint32_t task_container,
        std::uint32_t notification_argument,
        IBehaviorAtomicDispatchRuntime& runtime);
};

}  // namespace pal4
