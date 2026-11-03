#include "pal4/behavior_atomic_record_builder.h"

namespace pal4 {
namespace {

constexpr char kNodeName[] = "behAIAtomic-attribute";
constexpr char kClassName[] = "behAIAtomic-classID";
constexpr char kProportionName[] = "behAIAtomic-proportion";

bool StoreAttribute(std::uint32_t node, std::uint32_t key,
                    const char* name, std::int32_t value,
                    IBehaviorAtomicRecordRuntime& runtime) {
    const auto position = runtime.FindAttribute(node, key);
    runtime.CleanupKey(key);
    if (position.current != position.end &&
        position.current != 0xffffffe4u) {
        return runtime.SetAttributeInt(position.current + 28u, value) != 0u;
    }
    const auto attribute = runtime.InitializeAttribute(name, value);
    const auto result = runtime.AddAttribute(node, attribute);
    runtime.DestroyAttribute(attribute);
    return result != 0u;
}

}  // namespace

std::uint32_t BehaviorAtomicRecordRuntimeStub::InitializeNode(const char*) {
    return 0u;
}
std::uint32_t BehaviorAtomicRecordRuntimeStub::InitializeClassKey(
    const char*, std::uint8_t) { return 0u; }
std::uint32_t BehaviorAtomicRecordRuntimeStub::InitializeProportionKey(
    const char*, std::uint8_t) { return 0u; }
BehaviorAtomicAttributePosition BehaviorAtomicRecordRuntimeStub::FindAttribute(
    std::uint32_t, std::uint32_t) { return {}; }
void BehaviorAtomicRecordRuntimeStub::CleanupKey(std::uint32_t) {}
std::uint8_t BehaviorAtomicRecordRuntimeStub::SetAttributeInt(
    std::uint32_t, std::int32_t) { return 0u; }
std::uint32_t BehaviorAtomicRecordRuntimeStub::InitializeAttribute(
    const char*, std::int32_t) { return 0u; }
std::uint8_t BehaviorAtomicRecordRuntimeStub::AddAttribute(
    std::uint32_t, std::uint32_t) { return 0u; }
void BehaviorAtomicRecordRuntimeStub::DestroyAttribute(std::uint32_t) {}
void BehaviorAtomicRecordRuntimeStub::InsertNode(
    ScriptEventDataState&, std::uint32_t) {}
void BehaviorAtomicRecordRuntimeStub::DestroyNode(std::uint32_t) {}

std::uint32_t BehaviorAtomicRecordBuilder::AddClassProportion(
    ScriptEventDataState& state, std::int32_t class_id,
    std::int32_t proportion, std::uint8_t class_key_byte,
    std::uint8_t proportion_key_byte,
    IBehaviorAtomicRecordRuntime& runtime) {
    const auto node = runtime.InitializeNode(kNodeName);
    const auto class_key = runtime.InitializeClassKey(
        kClassName, class_key_byte);
    if (!StoreAttribute(node, class_key, kClassName, class_id, runtime)) {
        runtime.DestroyNode(node);
        return 0u;
    }
    const auto proportion_key = runtime.InitializeProportionKey(
        kProportionName, proportion_key_byte);
    if (!StoreAttribute(node, proportion_key, kProportionName,
                        proportion, runtime)) {
        runtime.DestroyNode(node);
        return 0u;
    }

    runtime.InsertNode(state, node);
    const auto begin = state.words[6];
    const auto end = state.words[7];
    const auto count = begin != 0u ? (end - begin) / 60u : 0u;
    const auto last = begin + 60u * count - 60u;
    runtime.DestroyNode(node);
    return last;
}

}  // namespace pal4
