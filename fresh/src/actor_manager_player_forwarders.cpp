#include "pal4/actor_manager_player_forwarders.h"

namespace pal4 {

std::uint32_t ActorManagerPlayerForwarders::SharedTextRecord(
    const std::uint32_t) {
    return 0x8fa36cu;
}

std::uint32_t ActorManagerPlayerForwarders::EmbeddedProperty144(
    const std::uint32_t actor) {
    return actor + 0x90u;
}

std::uint32_t ActorManagerPlayerForwarders::ForceActorModeOne(
    const std::uint32_t actor,
    IActorManagerPlayerParameterRuntime& runtime) {
    return runtime.ActorSlot48(actor, 1u);
}

std::uint32_t ActorManagerPlayerForwarders::Query37(
    const std::uint32_t output,
    IActorManagerPlayerForwarderRuntime& runtime) {
    const auto manager = runtime.SingletonManager();
    runtime.Query37(manager + 32u, output);
    return output;
}

std::uint32_t ActorManagerPlayerForwarders::Query38(
    const std::uint32_t output,
    IActorManagerPlayerForwarderRuntime& runtime) {
    const auto manager = runtime.SingletonManager();
    runtime.Query38(manager + 32u, output);
    return output;
}

std::uint32_t ActorManagerPlayerForwarders::FirstIterator(
    const std::uint32_t output,
    IActorManagerPlayerForwarderRuntime& runtime) {
    const auto manager = runtime.SingletonManager();
    runtime.FirstIterator(manager, output);
    return output;
}

std::uint32_t ActorManagerPlayerForwarders::SecondIterator(
    const std::uint32_t output,
    IActorManagerPlayerForwarderRuntime& runtime) {
    const auto manager = runtime.SingletonManager();
    runtime.SecondIterator(manager, output);
    return output;
}

}  // namespace pal4
