#include "pal4/actor_manager_player_callback_records.h"

namespace pal4 {
namespace {

std::uint32_t Destroy(const std::uint32_t object,
                      const std::uint32_t vtable,
                      const std::uint32_t flags,
                      IActorManagerPlayerCallbackRecordRuntime& runtime) {
    runtime.WriteWord(object, vtable);
    if ((flags & 1u) != 0u) runtime.Free(object);
    return object;
}

}  // namespace

std::uint32_t ActorManagerPlayerCallbackRecords::ConstructSetter(
    const std::uint32_t object, const std::uint32_t callback,
    const std::uint32_t context,
    IActorManagerPlayerCallbackRecordRuntime& runtime) {
    runtime.WriteWord(object, kSetterVtable);
    runtime.WriteWord(object + 8u, callback);
    runtime.WriteWord(object + 12u, context);
    return object;
}

void ActorManagerPlayerCallbackRecords::MarkFirst(
    const std::uint32_t object,
    IActorManagerPlayerCallbackRecordRuntime& runtime) {
    runtime.WriteWord(object, kFirstVtable);
}

void ActorManagerPlayerCallbackRecords::MarkSecond(
    const std::uint32_t object,
    IActorManagerPlayerCallbackRecordRuntime& runtime) {
    runtime.WriteWord(object, kSecondVtable);
}

std::uint32_t ActorManagerPlayerCallbackRecords::DestroyFirst(
    const std::uint32_t object, const std::uint32_t flags,
    IActorManagerPlayerCallbackRecordRuntime& runtime) {
    return Destroy(object, kFirstVtable, flags, runtime);
}

std::uint32_t ActorManagerPlayerCallbackRecords::DestroySecond(
    const std::uint32_t object, const std::uint32_t flags,
    IActorManagerPlayerCallbackRecordRuntime& runtime) {
    return Destroy(object, kSecondVtable, flags, runtime);
}

std::uint32_t ActorManagerPlayerCallbackRecords::DestroyFirstInline(
    const std::uint32_t object, const std::uint32_t flags,
    IActorManagerPlayerCallbackRecordRuntime& runtime) {
    return Destroy(object, kFirstVtable, flags, runtime);
}

std::uint32_t ActorManagerPlayerCallbackRecords::DestroySecondInline(
    const std::uint32_t object, const std::uint32_t flags,
    IActorManagerPlayerCallbackRecordRuntime& runtime) {
    return Destroy(object, kSecondVtable, flags, runtime);
}

}  // namespace pal4
