#include "pal4/combat_end_sound_events.h"

namespace pal4 {

std::uint32_t CombatEndSoundEvents::Initialize(
    const std::uint32_t owner, IEventSubscriptionRuntime& runtime) {
    Remove(owner, runtime);
    const auto first = runtime.Register(runtime.EventRouter(), 2u,
        kFirstDescriptor, owner, 0x579bd0u);
    runtime.WriteWord(owner, first);
    runtime.WriteWord(owner + 4u, kFirstDescriptor);
    const auto second = runtime.Register(runtime.EventRouter(), 2u,
        kSecondDescriptor, owner, 0x579d00u);
    runtime.WriteWord(owner + 12u, kSecondDescriptor);
    runtime.WriteWord(owner + 8u, second);
    return second;
}

void CombatEndSoundEvents::Remove(
    const std::uint32_t owner, IEventSubscriptionRuntime& runtime) {
    EventSubscriptionSet::Clear(owner, 0u, 2u, runtime);
}

std::uint32_t CombatEndSoundEvents::Construct(
    const std::uint32_t owner, IEventSubscriptionRuntime& runtime) {
    for (std::uint32_t index = 0u; index != 2u; ++index) {
        runtime.WriteWord(owner + 8u * index, 0u);
        runtime.WriteWord(owner + 8u * index + 4u, 0u);
    }
    static_cast<void>(Initialize(owner, runtime));
    return owner;
}

}  // namespace pal4
