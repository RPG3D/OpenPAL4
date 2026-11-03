#include "pal4/player_manager_leader_transition.h"

#include "pal4/player_manager_object.h"

namespace pal4 {

std::uint32_t PlayerManagerLeaderRuntimeStub::ReadDword(std::uint32_t) { return 0u; }
void PlayerManagerLeaderRuntimeStub::WriteDword(std::uint32_t, std::uint32_t) {}
void PlayerManagerLeaderRuntimeStub::SetStateFlag(std::uint32_t, std::uint32_t) {}
void PlayerManagerLeaderRuntimeStub::SetAction(std::uint32_t, std::uint32_t) {}
void PlayerManagerLeaderRuntimeStub::CopyTransform(
    std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) {}
void PlayerManagerLeaderRuntimeStub::SetPlayerState(
    std::uint32_t, std::uint32_t) {}
std::uint32_t PlayerManagerLeaderRuntimeStub::Arena() { return 0u; }
void PlayerManagerLeaderRuntimeStub::OnRemove(
    std::uint32_t, std::uint32_t) {}
void PlayerManagerLeaderRuntimeStub::OnAdd(
    std::uint32_t, std::uint32_t) {}
std::uint32_t PlayerManagerLeaderRuntimeStub::ActorControl() { return 0u; }
void PlayerManagerLeaderRuntimeStub::BindActor(
    std::uint32_t, std::uint32_t) {}
void PlayerManagerLeaderRuntimeStub::SortActive(
    std::vector<std::uint32_t>&, std::uint32_t) {}
std::int32_t PlayerManagerLeaderRuntimeStub::GetPriority(
    std::uint32_t) { return 0; }
void PlayerManagerLeaderRuntimeStub::SetPriority(
    std::uint32_t, std::int32_t) {}

void PlayerManagerLeaderTransition::Switch(
    const std::uint32_t manager, const std::uint32_t requested_index,
    IPlayerManagerLeaderRuntime& runtime) {
    const auto old_index = runtime.ReadDword(manager + kCurrentIndexOffset);
    if (old_index == requested_index) return;

    runtime.WriteDword(manager + kCurrentIndexOffset, requested_index);
    const auto old_slot = manager + old_index * PlayerManagerObject::kElementBytes;
    const auto new_slot = manager + requested_index * PlayerManagerObject::kElementBytes;
    if (runtime.ReadDword(new_slot + kMembershipOffset) == 0u) return;

    for (std::uint32_t index = 0u; index < PlayerManagerObject::kElementCount;
         ++index)
        runtime.SetStateFlag(
            manager + index * PlayerManagerObject::kElementBytes, 0u);
    runtime.SetStateFlag(new_slot, 1u);
    runtime.SetAction(new_slot + kActionOffset, kActionName);
    runtime.CopyTransform(new_slot, 0x4u, old_slot + 0xacu, 0u);
    runtime.CopyTransform(new_slot, 0x8u, old_slot + 0xa0u, 0u);
    runtime.SetPlayerState(manager,
                           runtime.ReadDword(old_slot + kPlayerStateOffset));
    runtime.OnRemove(runtime.Arena(), old_slot);
    runtime.OnAdd(runtime.Arena(), new_slot);
    runtime.BindActor(runtime.ActorControl(), new_slot);

    std::vector<std::uint32_t> active;
    active.reserve(PlayerManagerObject::kElementCount);
    for (std::uint32_t index = 0u; index < PlayerManagerObject::kElementCount;
         ++index) {
        const auto slot = manager + index * PlayerManagerObject::kElementBytes;
        if (slot != 0u && runtime.ReadDword(slot + kMembershipOffset) != 0u)
            active.push_back(slot);
    }
    runtime.SortActive(active, kComparator);

    const auto first_script = active.front() + kScriptOffset;
    const auto new_script = new_slot + kScriptOffset;
    const auto first_priority = runtime.GetPriority(first_script);
    const auto new_priority = runtime.GetPriority(new_script);
    runtime.SetPriority(first_script, new_priority);
    runtime.SetPriority(new_script, first_priority);
}

bool PlayerManagerLeaderTransition::ComparePlayerStates(
    const std::uint32_t left_slot, const std::uint32_t right_slot,
    IPlayerManagerPriorityRuntime& runtime) {
    const auto right_priority = runtime.GetPriority(right_slot + kScriptOffset);
    const auto left_priority = runtime.GetPriority(left_slot + kScriptOffset);
    return left_priority < right_priority;
}

}  // namespace pal4
