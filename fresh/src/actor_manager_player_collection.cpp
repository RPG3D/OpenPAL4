#include "pal4/actor_manager_player_collection.h"

#include <initializer_list>

namespace pal4 {
namespace {
void SyncPlayer(std::uint32_t actor,IActorManagerPlayerSyncRuntime& runtime) {
    const auto index=runtime.ReadWord(actor+1192u);
    const auto player=runtime.FindPlayer(runtime.PlayerManager(),index);
    if(player)runtime.CopyActorToPlayer(actor,player);
}
}

std::uint32_t IActorManagerPlayerCollectionRuntime::CreateActor(std::uint32_t player) {
    const auto storage=Allocate(0x528u);
    return storage?ConstructActor(storage,player):0u;
}
void IActorManagerPlayerCollectionRuntime::AppendActor(std::uint32_t collection,std::uint32_t actor) {
    InsertActor(collection,ReadWord(collection+8u),actor);
}

std::uint32_t ActorManagerPlayerCollection::Construct(
    const std::uint32_t collection,
    IActorManagerPlayerCollectionRuntime& runtime) {
    for (const auto offset : {0u, 16u}) {
        runtime.WriteByte(collection + offset, 0u);
        runtime.WriteWord(collection + offset + 4u, 0u);
        runtime.WriteWord(collection + offset + 8u, 0u);
        runtime.WriteWord(collection + offset + 12u, 0u);
    }
    RegisterEventHandlers(collection, runtime);
    return collection;
}

std::uint32_t ActorManagerPlayerCollection::Destroy(
    const std::uint32_t collection,
    IActorManagerPlayerCollectionRuntime& runtime) {
    static_cast<void>(Clear(collection, runtime));
    static_cast<void>(runtime.Free(runtime.ReadWord(collection + 20u)));
    for (const auto offset : {20u, 24u, 28u})
        runtime.WriteWord(collection + offset, 0u);
    const auto result = runtime.Free(runtime.ReadWord(collection + 4u));
    for (const auto offset : {4u, 8u, 12u})
        runtime.WriteWord(collection + offset, 0u);
    return result;
}

std::uint32_t ActorManagerPlayerCollection::AddPlayer(
    const std::uint32_t collection, const std::uint32_t player,
    const std::uint32_t state,
    IActorManagerPlayerAddRuntime& runtime) {
    if (player == 0u) {
        runtime.ReportNullPlayer("cbRoleManager::addRole",
                                 "role pointer is null");
        return 0u;
    }

    const auto actor = runtime.CreateActor(player);
    if (actor == 0u) return 0u;

    runtime.AssignActorName(actor, runtime.ActorBaseName(actor));
    runtime.AppendActor(collection,actor);
    const auto actor_state = state == 6u
        ? runtime.PlayerActionState(player) : state;
    runtime.SetActorState(actor, actor_state);
    runtime.DispatchNewActor("re_new_added", actor, 1u);
    return actor;
}

std::uint32_t ActorManagerPlayerCollection::SyncPlayers(
    const std::uint32_t collection,
    IActorManagerPlayerCollectionRuntime& runtime) {
    auto cursor = runtime.ReadWord(collection + 4u);
    std::uint32_t end = 0u;
    while (cursor != (end = runtime.ReadWord(collection + 8u))) {
        const auto actor = runtime.ReadWord(cursor);
        SyncPlayer(actor,runtime);
        cursor += 4u;
    }
    return end;
}
void ActorManagerPlayerCollection::SyncPlayers(std::span<const std::uint32_t> actors,
    IActorManagerPlayerSyncRuntime& runtime) {
    for(auto actor:actors)SyncPlayer(actor,runtime);
}

std::uint32_t ActorManagerPlayerCollection::FirstCandidateIterator(
    const std::uint32_t collection, const std::uint32_t output,
    IActorManagerPlayerCollectionRuntime& runtime) {
    const auto end = runtime.ReadWord(collection + 4u);
    runtime.WriteWord(output,
                      ActorManagerPlayerCandidateIterator::FromFields(
                          end, collection, runtime));
    return output;
}

std::uint32_t ActorManagerPlayerCollection::SecondCandidateIterator(
    const std::uint32_t collection, const std::uint32_t output,
    IActorManagerPlayerCollectionRuntime& runtime) {
    const auto source = collection + 16u;
    const auto end = runtime.ReadWord(source + 4u);
    runtime.WriteWord(output,
                      ActorManagerPlayerCandidateIterator::FromFields(
                          end, source, runtime));
    return output;
}

std::uint32_t ActorManagerPlayerCollection::FindSecondActor(
    const std::uint32_t collection, const std::uint32_t value,
    IActorManagerPlayerCollectionRuntime& runtime) {
    for (std::uint32_t index = 0u;; ++index) {
        const auto begin = runtime.ReadWord(collection + 20u);
        if (begin == 0u ||
            index >= (runtime.ReadWord(collection + 24u) - begin) / 4u)
            return 0u;
        const auto actor = runtime.ReadWord(begin + 4u * index);
        if (runtime.ActorValue20(actor) == value) return actor;
    }
}

std::uint32_t ActorManagerPlayerCollection::UpdateActors(
    const std::uint32_t collection, const std::uint32_t time_bits,
    IActorManagerPlayerCollectionRuntime& runtime) {
    std::uint32_t result = 0u;
    for (std::uint32_t index = 0u;; ++index) {
        const auto begin = runtime.ReadWord(collection + 20u);
        if (begin == 0u) break;
        result = (runtime.ReadWord(collection + 24u) - begin) / 4u;
        if (index >= result) break;
        runtime.CallChildSlot56(runtime.ReadWord(begin + 4u * index),
                                time_bits);
    }
    for (std::uint32_t index = 0u;; ++index) {
        const auto begin = runtime.ReadWord(collection + 4u);
        if (begin == 0u) break;
        result = (runtime.ReadWord(collection + 8u) - begin) / 4u;
        if (index >= result) break;
        const auto actor = runtime.ReadWord(begin + 4u * index);
        runtime.UpdateAnimationList(actor == 0u ? 0u : actor + 728u,
                                    time_bits);
    }
    return result;
}

std::uint32_t ActorManagerPlayerCollection::Clear(
    const std::uint32_t collection,
    IActorManagerPlayerCollectionRuntime& runtime) {
    for (auto cursor = runtime.ReadWord(collection + 4u);
         cursor != runtime.ReadWord(collection + 8u); cursor += 4u)
        runtime.ActorSlot388(runtime.ReadWord(cursor));

    const auto first_end = runtime.ReadWord(collection + 8u);
    for (auto cursor = runtime.ReadWord(collection + 4u);
         cursor != first_end; cursor += 4u)
        ReleaseActor(runtime.ReadWord(cursor), runtime);

    runtime.WriteWord(collection + 8u, runtime.ReadWord(collection + 4u));
    const auto second_end = runtime.ReadWord(collection + 24u);
    runtime.WriteWord(collection + 24u,
                      runtime.ReadWord(collection + 20u));
    return second_end;
}

void ActorManagerPlayerCollection::ReleaseActor(
    const std::uint32_t actor,
    IActorManagerPlayerCollectionRuntime& runtime) {
    if (actor == 0u) return;
    const auto name = runtime.ReadWord(actor + 816u);
    runtime.LogRelease(actor, name == 0u ? kEmptyName : name);
    runtime.ActorSlot248(actor, 1u);
}

void ActorManagerPlayerCollection::RegisterEventHandlers(
    const std::uint32_t collection,
    IActorManagerPlayerCollectionRuntime& runtime) {
    runtime.RegisterEvent(runtime.EventRouter(), 2u,
                          "re_join_stage", collection, 0x533950u);
    runtime.RegisterEvent(runtime.EventRouter(), 2u,
                          "re_leave_stage", collection, 0x53b580u);
    runtime.RegisterEvent(runtime.EventRouter(), 2u,
                          "combat_over", collection, 0x57b8d0u);
}

std::uint32_t ActorManagerPlayerCollection::OnLeaveStage(
    const std::uint32_t collection, const std::uint32_t parameter,
    IActorManagerPlayerCollectionRuntime& runtime) {
    auto cursor = runtime.ReadWord(collection + 20u);
    auto end = runtime.ReadWord(collection + 24u);
    while (cursor != end) {
        const auto value = runtime.DialogData(parameter);
        if (value == runtime.ReadWord(cursor)) {
            end = runtime.ReadWord(collection + 24u);
            for (auto source = cursor + 4u; source != end; source += 4u)
                runtime.WriteWord(source - 4u, runtime.ReadWord(source));
            runtime.WriteWord(collection + 24u, end - 4u);
            return end;
        }
        cursor += 4u;
        end = runtime.ReadWord(collection + 24u);
    }
    return end;
}

namespace {

std::uint32_t InitializeCandidateIterator(
    const std::uint32_t iterator, const std::uint32_t position,
    const std::uint32_t source,
    IActorManagerPlayerCollectionRuntime& runtime) {
    runtime.WriteWord(iterator,
                      ActorManagerPlayerCandidateIterator::kBaseVtable);
    runtime.WriteWord(iterator + 4u, position);
    runtime.WriteWord(iterator + 8u, source);
    runtime.WriteWord(iterator, ActorManagerPlayerCandidateIterator::kVtable);
    return iterator;
}

}  // namespace

std::uint32_t ActorManagerPlayerCandidateIterator::Clone(
    const std::uint32_t descriptor,
    IActorManagerPlayerCollectionRuntime& runtime) {
    const auto iterator = runtime.Allocate(12u);
    if (iterator == 0u) return 0u;
    runtime.WriteWord(iterator, kBaseVtable);
    runtime.WriteWord(iterator + 4u,
                      runtime.ReadWord(descriptor + 4u));
    runtime.WriteWord(iterator + 8u,
                      runtime.ReadWord(descriptor + 8u));
    runtime.WriteWord(iterator, kVtable);
    return iterator;
}

std::uint32_t ActorManagerPlayerCandidateIterator::FromFields(
    const std::uint32_t position, const std::uint32_t source,
    IActorManagerPlayerCollectionRuntime& runtime) {
    const auto iterator = runtime.Allocate(12u);
    if (iterator == 0u) return 0u;
    return InitializeCandidateIterator(iterator, position, source, runtime);
}

std::uint32_t ActorManagerPlayerCandidateIterator::Count(
    const std::uint32_t iterator,
    IActorManagerPlayerCollectionRuntime& runtime) {
    const auto source = runtime.ReadWord(iterator + 8u);
    const auto begin = runtime.ReadWord(source + 4u);
    if (begin == 0u) return 0u;
    const auto delta = runtime.ReadWord(source + 8u) - begin;
    return (delta >> 2u) |
           ((delta & 0x80000000u) != 0u ? 0xc0000000u : 0u);
}

bool ActorManagerPlayerCandidateIterator::AtBegin(
    const std::uint32_t iterator,
    IActorManagerPlayerCollectionRuntime& runtime) {
    const auto source = runtime.ReadWord(iterator + 8u);
    const auto position = runtime.ReadWord(iterator + 4u);
    return position == runtime.ReadWord(source + 4u);
}

bool ActorManagerPlayerCandidateIterator::AtEnd(
    const std::uint32_t iterator,
    IActorManagerPlayerCollectionRuntime& runtime) {
    const auto source = runtime.ReadWord(iterator + 8u);
    const auto position = runtime.ReadWord(iterator + 4u);
    return position == runtime.ReadWord(source + 8u);
}

std::uint32_t ActorManagerPlayerCandidateIterator::Next(
    const std::uint32_t iterator,
    IActorManagerPlayerCollectionRuntime& runtime) {
    const auto at_end = AtEnd(iterator, runtime);
    if (!at_end)
        runtime.WriteWord(iterator + 4u,
                          runtime.ReadWord(iterator + 4u) + 4u);
    return at_end ? 1u : 0u;
}

std::uint32_t ActorManagerPlayerCandidateIterator::Previous(
    const std::uint32_t iterator,
    IActorManagerPlayerCollectionRuntime& runtime) {
    const auto at_begin = AtBegin(iterator, runtime);
    if (!at_begin)
        runtime.WriteWord(iterator + 4u,
                          runtime.ReadWord(iterator + 4u) - 4u);
    return at_begin ? 1u : 0u;
}

std::uint32_t ActorManagerPlayerCandidateIterator::Delete(
    const std::uint32_t iterator, const std::uint8_t flags,
    IActorManagerPlayerCollectionRuntime& runtime) {
    runtime.WriteWord(iterator, kBaseVtable);
    if ((flags & 1u) != 0u) static_cast<void>(runtime.Free(iterator));
    return iterator;
}

}  // namespace pal4
