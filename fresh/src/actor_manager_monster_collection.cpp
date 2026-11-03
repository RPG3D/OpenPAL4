#include "pal4/actor_manager_monster_collection.h"

#include <initializer_list>
#include <stdexcept>
#include <string>

namespace pal4 {
namespace {

template <class ValueProvider>
std::uint32_t InsertValues(
    const std::uint32_t list, const std::uint32_t position,
    const std::uint32_t count, ValueProvider value,
    IActorManagerMonsterCollectionRuntime& runtime) {
    const auto begin = runtime.ReadWord(list + 4u);
    const auto end = runtime.ReadWord(list + 8u);
    const auto capacity_end = runtime.ReadWord(list + 12u);
    if (!count) return end;
    if (position < begin || position > end || ((position - begin) & 3u))
        throw std::invalid_argument("monster insertion position");

    const auto bytes = count * 4u;
    if (capacity_end >= end && (capacity_end - end) / 4u >= count) {
        for (auto cursor = end; cursor != position; cursor -= 4u)
            runtime.WriteWord(cursor + bytes - 4u,
                              runtime.ReadWord(cursor - 4u));
        for (std::uint32_t i = 0u; i < count; ++i)
            runtime.WriteWord(position + i * 4u, value());
        runtime.WriteWord(list + 8u, end + bytes);
        return end + bytes;
    }

    const auto size = begin ? (end - begin) / 4u : 0u;
    const auto growth = size > count ? size : count;
    const auto replacement = runtime.AllocateArray((size + growth) * 4u);
    if (!replacement) return 0u;
    const auto prefix = begin ? (position - begin) / 4u : 0u;
    for (std::uint32_t i = 0u; i < prefix; ++i)
        runtime.WriteWord(replacement + i * 4u,
                          runtime.ReadWord(begin + i * 4u));
    for (std::uint32_t i = 0u; i < count; ++i)
        runtime.WriteWord(replacement + (prefix + i) * 4u, value());
    for (std::uint32_t i = prefix; i < size; ++i)
        runtime.WriteWord(replacement + (i + count) * 4u,
                          runtime.ReadWord(begin + i * 4u));
    (void)runtime.Free(begin);
    runtime.WriteWord(list + 4u, replacement);
    runtime.WriteWord(list + 8u, replacement + (size + count) * 4u);
    runtime.WriteWord(list + 12u, replacement + (size + growth) * 4u);
    return replacement + (size + count) * 4u;
}

}  // namespace

std::uint32_t ActorManagerMonsterCollection::Construct(
    const std::uint32_t manager,
    IActorManagerMonsterCollectionRuntime& runtime) {
    for (const auto offset : {0u, 16u}) {
        runtime.WriteByte(manager + offset, 0u);
        runtime.WriteWord(manager + offset + 4u, 0u);
        runtime.WriteWord(manager + offset + 8u, 0u);
        runtime.WriteWord(manager + offset + 12u, 0u);
    }
    RegisterEventHandlers(manager, runtime);
    return manager;
}

std::uint32_t ActorManagerMonsterCollection::Destroy(
    const std::uint32_t manager,
    IActorManagerMonsterCollectionRuntime& runtime) {
    (void)Clear(manager, runtime);
    (void)runtime.Free(runtime.ReadWord(manager + 20u));
    for (const auto offset : {20u, 24u, 28u})
        runtime.WriteWord(manager + offset, 0u);
    const auto result = runtime.Free(runtime.ReadWord(manager + 4u));
    for (const auto offset : {4u, 8u, 12u})
        runtime.WriteWord(manager + offset, 0u);
    return result;
}

std::uint32_t ActorManagerMonsterCollection::CreateMonster(
    const std::uint32_t manager, const std::uint32_t key,
    const std::uint32_t state, const std::uint32_t group,
    IActorManagerMonsterCollectionRuntime& runtime) {
    const auto record = runtime.FindMonsterRecord(key);
    if (!record) {
        runtime.HandleMissingMonster(key);
        return 0u;
    }
    const auto storage = runtime.AllocateActor(0x490u);
    if (!storage) return 0u;
    const auto actor = runtime.ConstructActor(storage, record);
    if (!actor) return 0u;

    const auto begin = runtime.ReadWord(manager + 4u);
    const auto end = runtime.ReadWord(manager + 8u);
    const auto index = begin ? (end - begin) / 4u : 0u;
    std::string name = std::to_string(index);
    name += '_';
    name += runtime.ActorBaseName(actor);
    runtime.AssignActorName(actor, name);
    runtime.ActorSlot24(actor, state);
    const auto insertion = InsertValues(
        manager, runtime.ReadWord(manager + 8u), 1u,
        [actor] { return actor; }, runtime);
    if (!insertion) {
        ReleaseMonster(actor, runtime);
        return 0u;
    }
    runtime.DispatchNewActor(actor);
    if (group == 1u) runtime.ActorEvent268(actor, 17020u, actor);
    return actor;
}

std::uint32_t ActorManagerMonsterCollection::FirstIterator(
    const std::uint32_t manager, const std::uint32_t output,
    IActorManagerMonsterCollectionRuntime& runtime) {
    runtime.WriteWord(output,
                      runtime.CreateIterator(manager,
                                             runtime.ReadWord(manager + 4u)));
    return output;
}

std::uint32_t ActorManagerMonsterCollection::SecondIterator(
    const std::uint32_t manager, const std::uint32_t output,
    IActorManagerMonsterCollectionRuntime& runtime) {
    runtime.WriteWord(output,
                      runtime.CreateIterator(manager + 16u,
                                             runtime.ReadWord(manager + 20u)));
    return output;
}

std::uint32_t ActorManagerMonsterCollection::AverageProperty4(
    const std::uint32_t manager,
    IActorManagerMonsterCollectionRuntime& runtime) {
    std::uint32_t sum = 0u;
    for (auto cursor = runtime.ReadWord(manager + 4u);
         cursor != runtime.ReadWord(manager + 8u); cursor += 4u)
        sum += runtime.ActorProperty4(runtime.ReadWord(cursor));
    const auto begin = runtime.ReadWord(manager + 4u);
    const auto end = runtime.ReadWord(manager + 8u);
    const auto count = begin ? (end - begin) / 4u : 0u;
    if (!count) throw std::domain_error("monster average divides by zero");
    return sum / count;
}

std::uint32_t ActorManagerMonsterCollection::Clear(
    const std::uint32_t manager,
    IActorManagerMonsterCollectionRuntime& runtime) {
    for (auto cursor = runtime.ReadWord(manager + 4u);
         cursor != runtime.ReadWord(manager + 8u); cursor += 4u)
        runtime.ActorSlot388(runtime.ReadWord(cursor));
    const auto first_end = runtime.ReadWord(manager + 8u);
    for (auto cursor = runtime.ReadWord(manager + 4u);
         cursor != first_end; cursor += 4u)
        ReleaseMonster(runtime.ReadWord(cursor), runtime);
    runtime.WriteWord(manager + 8u, runtime.ReadWord(manager + 4u));
    const auto second_end = runtime.ReadWord(manager + 24u);
    runtime.WriteWord(manager + 24u, runtime.ReadWord(manager + 20u));
    return second_end;
}

void ActorManagerMonsterCollection::ReleaseMonster(
    const std::uint32_t actor,
    IActorManagerMonsterCollectionRuntime& runtime) {
    if (!actor) return;
    const auto name = runtime.ReadWord(actor + 816u);
    runtime.LogRelease(actor, name ? name : kEmptyName);
    runtime.ActorSlot248(actor, 1u);
}

void ActorManagerMonsterCollection::RegisterEventHandlers(
    const std::uint32_t manager,
    IActorManagerMonsterCollectionRuntime& runtime) {
    runtime.RegisterEvent(runtime.EventRouter(), 2u,
                          "me_join_stage", manager, 0x533950u);
    runtime.RegisterEvent(runtime.EventRouter(), 2u,
                          "me_leave_stage", manager, 0x53b580u);
    runtime.RegisterEvent(runtime.EventRouter(), 2u,
                          "combat_over", manager, 0x57b8d0u);
}

std::uint32_t ActorManagerMonsterCollection::OnJoinStage(
    const std::uint32_t manager, const std::uint32_t parameter,
    IActorManagerMonsterCollectionRuntime& runtime) {
    const auto value = runtime.DialogData(parameter);
    return InsertValues(manager + 16u, runtime.ReadWord(manager + 24u),
                        1u, [value] { return value; }, runtime);
}

std::uint32_t ActorManagerMonsterCollection::InsertCopies(
    const std::uint32_t manager, const std::uint32_t position,
    const std::uint32_t count, const std::uint32_t source,
    IActorManagerMonsterCollectionRuntime& runtime) {
    return InsertValues(manager, position, count,
                        [&runtime, source] { return runtime.ReadWord(source); },
                        runtime);
}

}  // namespace pal4
