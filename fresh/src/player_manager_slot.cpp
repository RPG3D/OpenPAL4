#include "pal4/player_manager_slot.h"

namespace pal4 {

void PlayerManagerSlotRuntimeStub::BaseConstruct(std::uint32_t) {}
void PlayerManagerSlotRuntimeStub::BaseReset(std::uint32_t) {}
std::uint32_t PlayerManagerSlotRuntimeStub::CleanupWeaponEffects(
    std::uint32_t) { return 0u; }
std::uint32_t PlayerManagerSlotRuntimeStub::BaseDestroy(
    std::uint32_t) { return 0u; }
void PlayerManagerSlotRuntimeStub::InitializeSixWords(std::uint32_t) {}
void PlayerManagerSlotRuntimeStub::ConstructBlockArray(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
std::uint32_t PlayerManagerSlotRuntimeStub::DestroyBlockArray(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }
void PlayerManagerSlotRuntimeStub::InitializeScript(std::uint32_t) {}
void PlayerManagerSlotRuntimeStub::CleanupScriptOnUnwind(
    std::uint32_t) {}
void PlayerManagerSlotRuntimeStub::ResetScriptVtable(
    std::uint32_t) {}
void PlayerManagerSlotRuntimeStub::InitializeResourceArrays(
    std::uint32_t) {}
void PlayerManagerSlotRuntimeStub::CleanupResourceArraysOnUnwind(
    std::uint32_t) {}
void PlayerManagerSlotRuntimeStub::DestroyResourceArray(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void PlayerManagerSlotRuntimeStub::InitializeTail(std::uint32_t) {}
void PlayerManagerSlotRuntimeStub::CleanupTail(std::uint32_t) {}
std::uint32_t PlayerManagerSlotRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void PlayerManagerSlotRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}
void PlayerManagerSlotRuntimeStub::WriteByte(
    std::uint32_t, std::uint8_t) {}
void PlayerManagerSlotRuntimeStub::Free(std::uint32_t) {}

std::uint32_t PlayerManagerSlot::Construct(
    std::uint32_t object, std::uint8_t incoming_stack_byte,
    IPlayerManagerSlotRuntime& runtime) {
    int unwind_state = -1;
    try {
        runtime.BaseConstruct(object);
        unwind_state = 0;
        runtime.InitializeSixWords(object + 0x920u);
        runtime.ConstructBlockArray(object + kFirstBlockArrayOffset, 5u, 16u);
        unwind_state = 1;
        runtime.ConstructBlockArray(object + kSecondBlockArrayOffset, 5u, 16u);

        runtime.WriteByte(object + 0x9d8u, incoming_stack_byte);
        runtime.WriteDword(object + 0x9dcu, 0u);
        runtime.WriteDword(object + 0x9e0u, 0u);
        runtime.WriteDword(object + 0x9e4u, 0u);
        runtime.WriteByte(object + 0x9e8u, incoming_stack_byte);
        runtime.WriteDword(object + 0x9ecu, 0u);
        runtime.WriteDword(object + 0x9f0u, 0u);
        runtime.WriteDword(object + 0x9f4u, 0u);

        unwind_state = 2;
        runtime.InitializeScript(object + kScriptOffset);
        unwind_state = 3;
        runtime.InitializeResourceArrays(object + kResourceArrayOffset);
        unwind_state = 4;
        runtime.InitializeTail(object + kTailOffset);

        runtime.WriteDword(object + kScriptOffset, kScriptVtable);
        runtime.WriteDword(object + 0xb08u, 0u);
        runtime.WriteDword(object + 0xb0cu, 1u);
        runtime.WriteDword(object, kMainVtable);
        runtime.WriteDword(object + 0x170u, kAnimationVtable);
        runtime.WriteDword(object + 0x1ecu, kSecondaryVtable);
        runtime.WriteDword(object + 0xb10u, 0u);
        return object;
    } catch (...) {
        if (unwind_state >= 4)
            runtime.CleanupResourceArraysOnUnwind(
                object + kResourceArrayOffset);
        if (unwind_state >= 3)
            runtime.CleanupScriptOnUnwind(object + kScriptOffset);
        if (unwind_state >= 2) {
            static_cast<void>(DestroyCollections(
                object + kFirstBlockArrayOffset, runtime));
        } else if (unwind_state >= 1) {
            static_cast<void>(runtime.DestroyBlockArray(
                object + kFirstBlockArrayOffset, 5u, 16u));
        }
        if (unwind_state >= 0)
            static_cast<void>(runtime.BaseDestroy(object));
        throw;
    }
}

std::uint32_t PlayerManagerSlot::DestroyCollections(
    std::uint32_t collections, IPlayerManagerSlotRuntime& runtime) {
    runtime.Free(runtime.ReadDword(collections + 0xb4u));
    runtime.WriteDword(collections + 0xb4u, 0u);
    runtime.WriteDword(collections + 0xb8u, 0u);
    runtime.WriteDword(collections + 0xbcu, 0u);
    runtime.Free(runtime.ReadDword(collections + 0xa4u));
    runtime.WriteDword(collections + 0xa4u, 0u);
    runtime.WriteDword(collections + 0xa8u, 0u);
    runtime.WriteDword(collections + 0xacu, 0u);
    static_cast<void>(runtime.DestroyBlockArray(
        collections + 0x50u, 5u, 16u));
    return runtime.DestroyBlockArray(collections, 5u, 16u);
}

std::uint32_t PlayerManagerSlot::DestroyDeleting(
    std::uint32_t object, std::uint32_t flags,
    IPlayerManagerSlotRuntime& runtime) {
    static_cast<void>(Destroy(object, runtime));
    if ((flags & 1u) != 0u) runtime.Free(object);
    return object;
}

std::uint32_t PlayerManagerSlot::DestroyDeletingFromScript(
    std::uint32_t script_subobject, std::uint32_t flags,
    IPlayerManagerSlotRuntime& runtime) {
    return DestroyDeleting(script_subobject - kScriptOffset, flags, runtime);
}

std::uint32_t PlayerManagerSlot::Destroy(
    std::uint32_t object, IPlayerManagerSlotRuntime& runtime) {
    const auto script = object + kScriptOffset;
    runtime.WriteDword(object, kMainVtable);
    runtime.WriteDword(object + 0x170u, kAnimationVtable);
    runtime.WriteDword(object + 0x1ecu, kSecondaryVtable);
    runtime.WriteDword(script, kScriptVtable);
    runtime.CleanupTail(object + kTailOffset);
    runtime.DestroyResourceArray(
        object + kResourceArrayOffset + 0x60u, 5u, 16u);
    runtime.DestroyResourceArray(
        object + kResourceArrayOffset, 4u, 16u);

    runtime.WriteDword(script, kScriptCleanupVtable);
    runtime.Free(runtime.ReadDword(script + 0x2b8u));
    runtime.WriteDword(script + 0x2b8u, 0u);
    runtime.WriteDword(script + 0x2bcu, 0u);
    runtime.WriteDword(script + 0x2c0u, 0u);
    runtime.Free(runtime.ReadDword(script + 0x2a8u));
    runtime.WriteDword(script + 0x2a8u, 0u);
    runtime.WriteDword(script + 0x2acu, 0u);
    runtime.WriteDword(script + 0x2b0u, 0u);
    runtime.ResetScriptVtable(script);
    static_cast<void>(DestroyCollections(
        object + kFirstBlockArrayOffset, runtime));
    return runtime.BaseDestroy(object);
}

std::uint32_t PlayerManagerSlot::Reset(
    std::uint32_t object, IPlayerManagerSlotRuntime& runtime) {
    runtime.BaseReset(object);
    for (std::uint32_t offset = 0xaecu; offset < 0xb08u; offset += 4u)
        runtime.WriteDword(object + offset, 0u);
    runtime.WriteDword(object + 0xb08u, 0u);
    return 0u;
}

std::uint32_t PlayerManagerSlot::ClearWeaponEffects(
    std::uint32_t object, IPlayerManagerSlotRuntime& runtime) {
    return runtime.CleanupWeaponEffects(object);
}

}  // namespace pal4
