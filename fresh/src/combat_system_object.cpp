#include "pal4/combat_system_object.h"

#include <new>

namespace pal4 {
namespace {

void ConstructMemberAt(const std::uint32_t object,
                       const std::uint32_t offset,
                       const std::uint32_t bytes,
                       const CombatSystemMember member,
                       ICombatSystemObjectRuntime& runtime) {
    const auto storage = runtime.Allocate(bytes);
    runtime.WriteWord(object + offset,
                      storage != 0u ? runtime.ConstructMember(member, storage)
                                    : 0u);
}

void DestroyMemberAt(const std::uint32_t object,
                     const std::uint32_t offset,
                     const CombatSystemMember member,
                     const bool invoke_destructor,
                     ICombatSystemObjectRuntime& runtime) {
    const auto pointer = runtime.ReadWord(object + offset);
    if (pointer == 0u) return;
    if (invoke_destructor) runtime.DestroyMember(member, pointer);
    static_cast<void>(runtime.Free(pointer));
}

}  // namespace

std::uint32_t CombatSystemObject::Construct(
    const std::uint32_t object, const std::uint8_t data_flag_seed,
    ICombatSystemObjectRuntime& runtime) {
    runtime.ConstructWorld(object);
    runtime.ConstructTranslator(object + 380u);
    ConstructMemberAt(object, 396u, 0x80u,
                      CombatSystemMember::system_manager, runtime);
    ConstructMemberAt(object, 400u, 0x234u,
                      CombatSystemMember::event_router, runtime);
    ConstructMemberAt(object, 404u, 0x8cu,
                      CombatSystemMember::script_handlers, runtime);
    ConstructMemberAt(object, 408u, 0x4cu,
                      CombatSystemMember::extended_handlers, runtime);

    const auto data = runtime.Allocate(0x18u);
    if (data != 0u) {
        runtime.WriteByte(data, data_flag_seed);
        const auto sentinel = runtime.Allocate(0x28u);
        if (sentinel == 0u) throw std::bad_alloc();
        runtime.WriteWord(sentinel, sentinel);
        runtime.WriteWord(sentinel + 4u, sentinel);
        runtime.WriteWord(data + 4u, sentinel);
        runtime.WriteWord(data + 8u, 0u);
        runtime.WriteByte(data + 12u, data_flag_seed);
        runtime.WriteWord(data + 16u, runtime.ConstructDataStructure());
        runtime.WriteWord(data + 20u, 0u);
    }
    runtime.WriteWord(object + 412u, data);
    ConstructMemberAt(object, 416u, 0x26cu,
                      CombatSystemMember::game_event_handler, runtime);
    ConstructMemberAt(object, 420u, 0x10u,
                      CombatSystemMember::auxiliary, runtime);
    ConstructMemberAt(object, 424u, 0x190u,
                      CombatSystemMember::actor_manager, runtime);
    runtime.WriteWord(object + 428u, 0u);
    runtime.InitializeTail(object + 432u);
    runtime.WriteWord(object + 504u, 3u);
    runtime.WriteWord(object + 508u, 7u);
    runtime.WriteByte(object + 516u, 0u);
    runtime.WriteByte(object + 517u, 0u);
    return object;
}

std::uint32_t CombatSystemObject::Destruct(
    const std::uint32_t object, ICombatSystemObjectRuntime& runtime) {
    DestroyMemberAt(object, 396u, CombatSystemMember::system_manager, true,
                    runtime);
    DestroyMemberAt(object, 400u, CombatSystemMember::event_router, true,
                    runtime);
    DestroyMemberAt(object, 404u, CombatSystemMember::script_handlers, false,
                    runtime);
    const auto data = runtime.ReadWord(object + 412u);
    if (data != 0u) {
        runtime.ClearCombatEvents(data + 12u);
        runtime.DestroyTimerQueue(data);
        static_cast<void>(runtime.Free(data));
    }
    DestroyMemberAt(object, 408u, CombatSystemMember::extended_handlers, false,
                    runtime);
    static_cast<void>(runtime.Free(runtime.ReadWord(object + 416u)));
    DestroyMemberAt(object, 420u, CombatSystemMember::auxiliary, false,
                    runtime);
    const auto active = runtime.ReadWord(object + 424u);
    if (active != 0u) runtime.DestroyVirtual(active, 1u);
    static_cast<void>(CombatSystemTextMembers::ReleaseAt(object + 488u,
                                                          runtime));
    static_cast<void>(CombatSystemTextMembers::ReleaseAt(object + 472u,
                                                          runtime));
    static_cast<void>(CombatSystemTextMembers::ReleaseAt(object + 456u,
                                                          runtime));
    runtime.DestructTranslator(object + 380u);
    return runtime.DestructWorld(object);
}

std::uint8_t CombatSystemObject::IsInactive(
    const std::uint32_t object, ICombatSystemObjectRuntime& runtime) {
    return runtime.ReadByte(object + 516u);
}

bool CombatSystemObject::IsActive(
    const std::uint32_t object, ICombatSystemObjectRuntime& runtime) {
    const auto state = static_cast<std::int32_t>(
        runtime.ReadWord(object + 508u));
    return state >= 2 && state <= 5;
}

bool CombatSystemObject::IsEnding(
    const std::uint32_t object, ICombatSystemObjectRuntime& runtime) {
    const auto state = static_cast<std::int32_t>(
        runtime.ReadWord(object + 508u));
    return state >= 4 && state <= 5;
}

std::uint32_t CombatSystemObject::GetCamera(
    const std::uint32_t object, ICombatSystemObjectRuntime& runtime) {
    return runtime.ReadWord(object + 424u);
}

std::uint8_t CombatSystemObject::AreAllObjectsReady(
    const std::uint32_t object, ICombatSystemObjectRuntime& runtime) {
    const auto system_manager = runtime.ReadWord(object + 396u);
    return system_manager == 0u ? 0u :
        runtime.AreAllObjectsReady(system_manager);
}

std::uint32_t CombatSystemObject::SetProperty110(
    const std::uint32_t object, const std::uint32_t value,
    ICombatSystemObjectRuntime& runtime) {
    runtime.WriteWord(object + 440u, value);
    return value;
}

std::uint8_t CombatSystemObject::GetProperty519(
    const std::uint32_t object, ICombatSystemObjectRuntime& runtime) {
    return runtime.ReadByte(object + 519u);
}

}  // namespace pal4
