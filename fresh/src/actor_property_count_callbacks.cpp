#include "pal4/actor_property_count_callbacks.h"

namespace pal4 {
namespace {

std::uint8_t Allows(std::uint32_t object, std::int32_t first,
                    std::int32_t mode, std::int32_t denied_a,
                    std::int32_t denied_b,
                    IActorPropertyCountCallbackRuntime& runtime) {
    if (mode == denied_a || mode == denied_b) return 0u;
    const auto owner = runtime.ReadWord(object + 8u);
    if (owner == 0u) return 1u;
    return static_cast<std::uint8_t>(
        runtime.OwnerAllows(owner, 8u, first, mode));
}

}  // namespace

std::uint8_t ActorPropertyCountCallbacks::AllowsExceptThreeFour(
    std::uint32_t object, std::int32_t first, std::int32_t mode,
    IActorPropertyCountCallbackRuntime& runtime) {
    return Allows(object, first, mode, 3, 4, runtime);
}

std::uint8_t ActorPropertyCountCallbacks::AllowsExceptTwoFour(
    std::uint32_t object, std::int32_t first, std::int32_t mode,
    IActorPropertyCountCallbackRuntime& runtime) {
    return Allows(object, first, mode, 2, 4, runtime);
}

std::int32_t ActorPropertyCountCallbacks::Constant98() {
    return 98;
}

std::int32_t ActorPropertyCountCallbacks::ReadOwnerValue(
    std::uint32_t object, std::int32_t first, std::int32_t mode,
    IActorPropertyCountCallbackRuntime& runtime) {
    const auto owner = runtime.ReadWord(object + 8u);
    const auto value = runtime.OwnerValue(owner, 148u, first, mode);
    if (mode != 3) return value;
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(value) * 2u);
}

std::int32_t ActorPropertyCountCallbacks::Constant99() {
    return 99;
}

std::uint32_t ActorPropertyCountCallbacks::Construct(
    std::uint32_t object, IActorPropertyCountCallbackRuntime& runtime) {
    runtime.WriteByte(object + 4u, 0u);
    runtime.WriteWord(object + 8u, 0u);
    runtime.WriteWord(object + 12u, 0u);
    runtime.WriteWord(object, 0x846290u);
    return object;
}

}  // namespace pal4
