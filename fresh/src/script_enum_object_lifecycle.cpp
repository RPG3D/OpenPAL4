#include "pal4/script_enum_object_lifecycle.h"

#include <array>
#include <stdexcept>

namespace pal4 {
namespace {

void ClearAdapter(ScriptEnumObjectState& state, std::uint32_t flag_word,
                  IScriptEnumObjectLifecycleRuntime& runtime) {
    const auto pointer = state.words[flag_word + 1u];
    if (pointer != 0u) {
        if (static_cast<std::uint8_t>(state.words[flag_word]) != 0u)
            runtime.DestroyAdapter(pointer, 1u);
        state.words[flag_word] &= 0xffffff00u;
    }
    state.words[flag_word + 1u] = 0u;
}

std::uint8_t DestroyBinding(ScriptEnumObjectState& state,
                            std::uint32_t derived_vtable,
                            IScriptEnumObjectLifecycleRuntime& runtime) {
    state.words[0] = derived_vtable;
    ClearAdapter(state, 1u, runtime);
    ClearAdapter(state, 3u, runtime);

    const auto data = state.words[6];
    if (data != 0u) {
        const auto reference = runtime.ReadByte(data - 1u);
        if (reference == 0u || reference == 0xffu)
            runtime.Free(data - 1u);
        else
            runtime.WriteByte(data - 1u,
                              static_cast<std::uint8_t>(reference - 1u));
    }
    state.words[6] = 0u;
    state.words[7] = 0u;
    state.words[8] = 0u;
    const auto result = static_cast<std::uint8_t>(state.words[1]);
    state.words[0] = ScriptEnumObjectLifecycle::kBaseVtable;
    return result;
}

}  // namespace

std::uint32_t ScriptEnumObjectLifecycle::ConstructEntry(
    ScriptEnumObjectState& state, std::uint32_t object_address) {
    state.words[1] &= 0xffffff00u;
    state.words[2] = 0u;
    state.words[3] &= 0xffffff00u;
    state.words[4] = 0u;
    state.words[5] = (state.words[5] & 0xffffff00u) |
                     (object_address >> 24u);
    state.words[6] = 0u;
    state.words[7] = 0u;
    state.words[8] = 0u;
    state.words[0] = 0x8481d0u;
    return object_address;
}

ScriptEnumAdapterPairState& ScriptEnumObjectLifecycle::InitializeAdapterPair(
    ScriptEnumAdapterPairState& state) {
    state.words[0] &= 0xffffff00u;
    state.words[1] = 0u;
    state.words[2] &= 0xffffff00u;
    state.words[3] = 0u;
    return state;
}

std::uint8_t ScriptEnumObjectLifecycle::DestroyValueBinding(
    ScriptEnumObjectState& state,
    IScriptEnumObjectLifecycleRuntime& runtime) {
    return DestroyBinding(state, 0x848200u, runtime);
}

std::uint8_t ScriptEnumObjectLifecycle::DestroyReferenceBinding(
    ScriptEnumObjectState& state,
    IScriptEnumObjectLifecycleRuntime& runtime) {
    return DestroyBinding(state, 0x8481d0u, runtime);
}

std::uint8_t ScriptEnumObjectLifecycle::DestroyAdapterPairPrimary(
    ScriptEnumAdapterPairState& state,
    IScriptEnumObjectLifecycleRuntime& runtime) {
    return CleanupAdapterPair(state, runtime);
}

std::uint8_t ScriptEnumObjectLifecycle::DestroyAdapterPairSecondary(
    ScriptEnumAdapterPairState& state,
    IScriptEnumObjectLifecycleRuntime& runtime) {
    return CleanupAdapterPair(state, runtime);
}

std::uint8_t ScriptEnumObjectLifecycle::DestroyAdapterPairAt(
    std::uint32_t native_entry, ScriptEnumAdapterPairState& state,
    IScriptEnumObjectLifecycleRuntime& runtime) {
    constexpr std::array<std::uint32_t, 11> kEntries{{
        0x586b20u, 0x586bf0u, 0x586cc0u, 0x586d90u,
        0x586e60u, 0x586f30u, 0x587000u, 0x5870d0u,
        0x5871a0u, 0x587270u, 0x587340u}};
    bool found = false;
    for (auto entry : kEntries) {
        if (entry == native_entry) {
            found = true;
            break;
        }
    }
    if (!found) throw std::invalid_argument("unknown adapter-pair destructor");
    return CleanupAdapterPair(state, runtime);
}

std::uint32_t ScriptEnumObjectLifecycle::BindingNameAddress(
    std::uint32_t object_address) {
    return StringSubobjectAddress(object_address);
}

std::uint8_t ScriptEnumObjectLifecycleRuntimeStub::ReadByte(
    std::uint32_t) { return 0u; }
void ScriptEnumObjectLifecycleRuntimeStub::WriteByte(
    std::uint32_t, std::uint8_t) {}
std::uint32_t ScriptEnumObjectLifecycleRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void ScriptEnumObjectLifecycleRuntimeStub::Free(std::uint32_t) {}
void ScriptEnumObjectLifecycleRuntimeStub::DestroyAdapter(
    std::uint32_t, std::uint32_t) {}
void ScriptEnumObjectLifecycleRuntimeStub::CleanupEmptyTemporary() {}
void ScriptEnumObjectLifecycleRuntimeStub::DestroyArray(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
std::uint32_t ScriptEnumForwardRuntimeStub::Invoke(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }

std::uint32_t ScriptEnumObjectLifecycle::StringSubobjectAddress(
    std::uint32_t object_address) {
    return object_address + 20u;
}

void ScriptEnumObjectLifecycle::Destroy(
    ScriptEnumObjectState& state,
    IScriptEnumObjectLifecycleRuntime& runtime) {
    const auto data = state.words[6];
    if (data != 0u) {
        const auto reference = runtime.ReadByte(data - 1u);
        if (reference == 0u || reference == 0xffu)
            runtime.Free(data - 1u);
        else
            runtime.WriteByte(data - 1u,
                              static_cast<std::uint8_t>(reference - 1u));
    }
    state.words[6] = 0u;
    state.words[7] = 0u;
    state.words[8] = 0u;

    ClearAdapter(state, 1u, runtime);
    ClearAdapter(state, 3u, runtime);
    runtime.CleanupEmptyTemporary();
    state.words[0] = kBaseVtable;
}

std::uint32_t ScriptEnumObjectLifecycle::DeletingDestroy(
    ScriptEnumObjectState& state, std::uint32_t object_address,
    std::uint8_t flags, IScriptEnumObjectLifecycleRuntime& runtime) {
    if ((flags & 2u) != 0u) {
        const auto base = object_address - 4u;
        const auto count = runtime.ReadDword(base);
        runtime.DestroyArray(object_address, count, 36u);
        if ((flags & 1u) != 0u)
            runtime.Free(base);
        return base;
    }

    Destroy(state, runtime);
    if ((flags & 1u) != 0u)
        runtime.Free(object_address);
    return object_address;
}

std::uint8_t ScriptEnumObjectLifecycle::CleanupAdapterPair(
    ScriptEnumAdapterPairState& state,
    IScriptEnumObjectLifecycleRuntime& runtime) {
    for (std::uint32_t flag_word : {0u, 2u}) {
        const auto pointer = state.words[flag_word + 1u];
        if (pointer != 0u) {
            if (static_cast<std::uint8_t>(state.words[flag_word]) != 0u)
                runtime.DestroyAdapter(pointer, 1u);
            state.words[flag_word] &= 0xffffff00u;
        }
        state.words[flag_word + 1u] = 0u;
    }
    return static_cast<std::uint8_t>(state.words[0]);
}

std::uint32_t ScriptEnumObjectLifecycle::DeleteBase(
    ScriptEnumObjectState& state, std::uint32_t object_address,
    std::uint8_t flags, IScriptEnumObjectLifecycleRuntime& runtime) {
    state.words[0] = kBaseVtable;
    if ((flags & 1u) != 0u)
        runtime.Free(object_address);
    return object_address;
}

std::uint32_t ScriptEnumObjectLifecycle::ForwardSlot12(
    std::uint32_t source_object, std::uint32_t target,
    IScriptEnumForwardRuntime& runtime) {
    return runtime.Invoke(target, 12u, source_object);
}

std::uint32_t ScriptEnumObjectLifecycle::ForwardSlot36(
    std::uint32_t source_object, std::uint32_t target,
    IScriptEnumForwardRuntime& runtime) {
    return runtime.Invoke(target, 36u, source_object);
}

}  // namespace pal4
