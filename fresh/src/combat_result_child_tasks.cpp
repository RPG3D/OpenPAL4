#include "pal4/combat_result_child_tasks.h"

namespace pal4 {

std::uint32_t FendOffImpactChildTask::Create(
    ICombatResultChildRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    try {
        runtime.ConstructBase(object);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
    try {
        runtime.WriteWord(object, kVtable);
    } catch (...) {
        runtime.DestroyBase(object);
        runtime.Free(object);
        throw;
    }
    return object;
}

std::uint32_t MissMessageChildTask::Create(
    ICombatResultChildRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    try {
        runtime.ConstructBase(object);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
    try {
        runtime.WriteWord(object + 60u, 0u);
        // The native constructor copies an unwritten stack byte here.
        runtime.WriteByte(object + 64u, 0u);
        runtime.WriteWord(object + 68u, 0u);
        runtime.WriteWord(object + 72u, 0u);
        runtime.WriteWord(object + 76u, 0u);
        runtime.WriteWord(object, kVtable);
    } catch (...) {
        runtime.DestroyBase(object);
        runtime.Free(object);
        throw;
    }
    return object;
}

std::uint32_t FleeCandidateChildTask::Create(
    ICombatResultChildRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    try {
        runtime.ConstructBase(object);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
    try {
        runtime.WriteWord(object + 60u, 0u);
        // The native constructor copies an unwritten stack byte here.
        runtime.WriteByte(object + 64u, 0u);
        runtime.WriteWord(object + 68u, 0u);
        runtime.WriteWord(object + 72u, 0u);
        runtime.WriteWord(object + 76u, 0u);
        runtime.WriteByte(object + 80u, 0u);
        runtime.WriteWord(object, kVtable);
    } catch (...) {
        runtime.DestroyBase(object);
        runtime.Free(object);
        throw;
    }
    return object;
}

std::uint32_t FleeCandidateChildTask::DestroyBase(
    const std::uint32_t object,
    IFleeCandidateChildDestructionRuntime& runtime) {
    const auto text = runtime.ReadWord(object + 68u);
    if (text != 0u) {
        const auto header = text - 1u;
        const auto count = runtime.ReadByte(header);
        if (count == 0u || count == 0xffu)
            runtime.FreeText(header);
        else
            runtime.WriteByte(header, static_cast<std::uint8_t>(count - 1u));
    }
    runtime.WriteWord(object + 68u, 0u);
    runtime.WriteWord(object + 72u, 0u);
    runtime.WriteWord(object + 76u, 0u);
    return runtime.DestroyGenericBase(object);
}

std::uint32_t FleeFinishChildTask::Create(
    ICombatResultChildRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    try {
        runtime.ConstructBase(object);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
    try {
        runtime.WriteWord(object, kVtable);
    } catch (...) {
        runtime.DestroyBase(object);
        runtime.Free(object);
        throw;
    }
    return object;
}

}  // namespace pal4
