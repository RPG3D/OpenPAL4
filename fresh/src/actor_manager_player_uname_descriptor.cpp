#include "pal4/actor_manager_player_uname_descriptor.h"

namespace pal4 {

void ActorManagerPlayerUNameDescriptor::Destroy(
    const std::uint32_t descriptor,
    IActorManagerStringMethodRuntime& runtime) {
    ActorManagerStringMethodProperty::DestroyWithVtable(
        descriptor, kDerivedVtable, runtime);
}

}  // namespace pal4
