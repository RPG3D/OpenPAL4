#include "pal4/behavior_atomic_dispatcher.h"

#include <bit>
#include <stdexcept>

namespace pal4 {
namespace {

constexpr char kClassName[] = "behAIAtomic-classID";
constexpr char kProportionName[] = "behAIAtomic-proportion";
constexpr char kWaitModeName[] = "beh-vtm-waitMode";
constexpr char kWaitTimeName[] = "beh-vtm-waitTime";
constexpr char kExpectTimeName[] = "beh-vtm-expectTime";
constexpr char kAtomicName[] = "beh-vtm-atomic";
constexpr float kRandScale = std::bit_cast<float>(0x38000100u);
constexpr float kPercentScale = std::bit_cast<float>(0x3c23d70au);

std::int32_t ReadOptionalInt(std::uint32_t record, const char* name,
                             std::int32_t fallback,
                             IBehaviorAtomicDispatchRuntime& runtime) {
    const auto attribute = runtime.FindAttribute(record, name);
    return attribute != 0u ? runtime.ToInt(attribute, fallback) : fallback;
}

float ReadOptionalFloat(std::uint32_t record, const char* name,
                        IBehaviorAtomicDispatchRuntime& runtime) {
    const auto attribute = runtime.FindAttribute(record, name);
    return attribute != 0u ? runtime.ToFloat(attribute, 0.0f) : 0.0f;
}

void DispatchRecord(std::uint32_t record, std::uint32_t context,
                    std::uint32_t task_container,
                    std::uint32_t notification_argument,
                    IBehaviorAtomicDispatchRuntime& runtime) {
    const auto class_attribute = runtime.FindAttribute(record, kClassName);
    if (class_attribute == 0u) return;
    const auto class_id = runtime.ToInt(class_attribute, 0);
    if (class_id < 0) return;

    const BehaviorAtomicWaitOptions options{
        ReadOptionalInt(record, kWaitModeName, 3, runtime),
        ReadOptionalFloat(record, kWaitTimeName, runtime),
        ReadOptionalFloat(record, kExpectTimeName, runtime)};
    const auto task = runtime.ProcessEvent(
        context, class_id, task_container, kAtomicName, options);
    if (task != 0u)
        runtime.NotifyTask(task, notification_argument, record);
}

}  // namespace

void BehaviorAtomicDispatchRuntimeStub::CleanupCollection(std::uint32_t) {}
std::uint32_t BehaviorAtomicDispatchRuntimeStub::Rand() { return 0u; }
void BehaviorAtomicDispatchRuntimeStub::DivideByZero() {
    throw std::domain_error("empty behavior atomic selection");
}
std::uint32_t BehaviorAtomicDispatchRuntimeStub::FindAttribute(
    std::uint32_t, const char*) { return 0u; }
std::int32_t BehaviorAtomicDispatchRuntimeStub::ToInt(
    std::uint32_t, std::int32_t fallback) { return fallback; }
float BehaviorAtomicDispatchRuntimeStub::ToFloat(
    std::uint32_t, float fallback) { return fallback; }
std::uint32_t BehaviorAtomicDispatchRuntimeStub::ProcessEvent(
    std::uint32_t, std::int32_t, std::uint32_t, const char*,
    const BehaviorAtomicWaitOptions&) { return 0u; }
void BehaviorAtomicDispatchRuntimeStub::NotifyTask(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void BehaviorAtomicDispatchRuntimeStub::MarkProcessed(std::uint32_t) {}

std::uint8_t BehaviorAtomicDispatcher::Dispatch(
    ScriptEventDataState& state, std::uint32_t context,
    std::uint32_t task_container,
    std::uint32_t notification_argument,
    IBehaviorAtomicDispatchRuntime& runtime) {
    runtime.CleanupCollection(context);
    const auto begin = state.words[6];
    const auto end = state.words[7];
    const auto mode = state.words[4];

    if (mode == 0u) {
        for (auto record = begin; record != end; record += 60u)
            DispatchRecord(record, context, task_container,
                           notification_argument, runtime);
        runtime.MarkProcessed(context);
        return 1u;
    }

    std::uint32_t selected = 0u;
    if (mode == 1u) {
        const auto sample = static_cast<float>(
            static_cast<double>(static_cast<std::int32_t>(runtime.Rand())) *
            kRandScale);
        float cumulative = 0.0f;
        bool found = false;
        for (auto record = begin; record != end; record += 60u) {
            const auto attribute = runtime.FindAttribute(
                record, kProportionName);
            if (attribute == 0u) return 0u;
            const auto proportion = runtime.ToInt(attribute, 0);
            cumulative = static_cast<float>(
                static_cast<double>(proportion) * kPercentScale + cumulative);
            if (sample <= cumulative) {
                selected = record;
                found = true;
                break;
            }
        }
        if (!found) return 0u;
    } else if (mode == 2u) {
        const auto count = begin != 0u ? (end - begin) / 60u : 0u;
        if (begin != 0u && count == 1u) {
            selected = begin;
        } else {
            const auto sample = runtime.Rand();
            if (count == 0u) runtime.DivideByZero();
            selected = begin + 60u * (sample % count);
        }
    } else {
        return 0u;
    }

    DispatchRecord(selected, context, task_container,
                   notification_argument, runtime);
    runtime.MarkProcessed(context);
    return 1u;
}

}  // namespace pal4
