#include "pal4/script_registration_catalog.h"

#include <stdexcept>

namespace pal4 {

std::size_t ScriptRegistrationCatalog::Index(ScriptRegistrationKind kind) {
    const auto index = static_cast<std::size_t>(kind);
    if (index >= kCleanupCallbacks.size()) {
        throw std::out_of_range("script registration kind");
    }
    return index;
}

ScriptRegistrationQueueState& ScriptRegistrationCatalog::GetOrInit(
    ScriptRegistrationKind kind, std::uint8_t native_seed_byte,
    IScriptRegistrationCatalogRuntime& runtime) {
    const auto index = Index(kind);
    auto& queue = queues_[index];
    if ((guards_[index] & 1u) == 0u) {
        queue.seed_byte = native_seed_byte;
        guards_[index] |= 1u;
        queue.begin = 0u;
        queue.end = 0u;
        queue.capacity_end = 0u;
        runtime.RegisterAtExit(kCleanupCallbacks[index]);
    }
    return queue;
}

void ScriptRegistrationCatalog::Cleanup(
    ScriptRegistrationKind kind, IScriptRegistrationCatalogRuntime& runtime) {
    auto& queue = queues_[Index(kind)];
    runtime.Free(queue.begin);
    queue.begin = 0u;
    queue.end = 0u;
    queue.capacity_end = 0u;
}

ScriptRegistrationQueueState& ScriptRegistrationCatalog::ClearAll(
    const std::array<std::uint8_t, 3>& native_seed_bytes,
    IScriptRegistrationCatalogRuntime& runtime) {
    ScriptRegistrationQueueState* last = nullptr;
    for (std::size_t index = 0; index < queues_.size(); ++index) {
        auto& queue = GetOrInit(static_cast<ScriptRegistrationKind>(index),
                                native_seed_bytes[index], runtime);
        queue.end = queue.begin;
        last = &queue;
    }
    return *last;
}

void ScriptRegistrationCatalog::RegisterGlobalFunction(
    std::uint32_t descriptor, std::uint8_t native_seed_byte,
    IScriptRegistrationCatalogRuntime& runtime) {
    RegisterDescriptor(ScriptRegistrationKind::GlobalFunctions, descriptor,
                       native_seed_byte, runtime);
}

void ScriptRegistrationCatalog::RegisterGlobalVariable(
    std::uint32_t descriptor, std::uint8_t native_seed_byte,
    IScriptRegistrationCatalogRuntime& runtime) {
    RegisterDescriptor(ScriptRegistrationKind::GlobalVariables, descriptor,
                       native_seed_byte, runtime);
}

void ScriptRegistrationCatalog::RegisterDescriptor(
    ScriptRegistrationKind kind, std::uint32_t descriptor,
    std::uint8_t native_seed_byte, IScriptRegistrationCatalogRuntime& runtime) {
    auto& queue = GetOrInit(kind, native_seed_byte, runtime);
    const auto name = runtime.ReadPointer(descriptor + 8u);
    for (auto slot = queue.begin; slot != queue.end; slot += 4u) {
        const auto existing = runtime.ReadPointer(slot);
        auto left = name;
        auto right = runtime.ReadPointer(existing + 8u);
        for (;;) {
            const auto a = runtime.ReadByte(left++);
            const auto b = runtime.ReadByte(right++);
            if (a != b) {
                break;
            }
            if (a == 0u) {
                return;
            }
        }
    }

    if (queue.end != queue.capacity_end) {
        runtime.WritePointer(queue.end, descriptor);
        queue.end += 4u;
        return;
    }

    const auto count = queue.begin == 0u ? 0u : (queue.end - queue.begin) / 4u;
    const auto growth = count > 1u ? count : 1u;
    const auto new_begin = runtime.Allocate((count + growth) * 4u);
    for (std::uint32_t index = 0; index < count; ++index) {
        runtime.WritePointer(new_begin + index * 4u,
                             runtime.ReadPointer(queue.begin + index * 4u));
    }
    runtime.WritePointer(new_begin + count * 4u, descriptor);
    runtime.Free(queue.begin);
    queue.begin = new_begin;
    queue.end = new_begin + (count + 1u) * 4u;
    queue.capacity_end = new_begin + (count + growth) * 4u;
}

std::uint8_t ScriptRegistrationCatalog::Guard(
    ScriptRegistrationKind kind) const {
    return guards_[Index(kind)];
}

void ScriptRegistrationCatalog::SetGuard(
    ScriptRegistrationKind kind, std::uint8_t value) {
    guards_[Index(kind)] = value;
}

}  // namespace pal4
