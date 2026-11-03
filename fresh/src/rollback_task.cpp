#include "pal4/rollback_task.h"

namespace pal4 {

std::uint32_t RollbackTask::Construct(
    const std::uint32_t object, IRollbackTaskRuntime& runtime) {
    runtime.BaseConstruct(object);
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0x40u, 0u);
    return object;
}

std::uint8_t RollbackTask::Enter(
    const std::uint32_t object, IRollbackTaskRuntime& runtime) {
    const auto collection = runtime.ReadWord(object + 0x40u);
    if (collection != 0u) runtime.CleanupCollection(collection);
    const auto owner_collection =
        runtime.ReadWord(object + 0x3cu) + 0x2a0u;
    runtime.WriteWord(owner_collection + 8u,
                      runtime.ReadWord(owner_collection + 4u));
    const auto previous = runtime.ReadWord(object + 0x30u);
    if (previous != 0u) runtime.NotifyPrevious(previous);
    return 1u;
}

std::uint8_t RollbackTask::ReturnZeroWithArgument(
    const std::uint32_t) {
    return 0u;
}

std::uint8_t RollbackTask::ReturnZero() {
    return 0u;
}

bool RollbackTask::WaitUntilSiblingsDone(
    const std::uint32_t object, IRollbackTaskRuntime& runtime) {
    if (runtime.ReadWord(object + 0x28u) != 2u) return false;
    const auto group = runtime.ReadWord(object + 0x38u);
    if (group == 0u) return false;
    auto remaining = runtime.ReadWord(group + 8u);
    for (auto sibling = runtime.ReadWord(object + 0x30u);
         sibling != 0u; sibling = runtime.ReadWord(sibling + 0x30u)) {
        if (runtime.SiblingComplete(sibling)) --remaining;
    }
    for (auto sibling = runtime.ReadWord(object + 0x34u);
         sibling != 0u; sibling = runtime.ReadWord(sibling + 0x34u)) {
        if (runtime.SiblingComplete(sibling)) --remaining;
    }
    return remaining == 1u && runtime.WaitComplete(object);
}

}  // namespace pal4
