#include "pal4/player_manager_access.h"

namespace pal4 {

std::uint32_t PlayerManagerAccessRuntimeStub::SingletonManager() {
    return 0u;
}
std::uint32_t PlayerManagerAccessRuntimeStub::Register(
    std::uint32_t, std::string_view, std::uint32_t,
    std::uint32_t, std::uint32_t) {
    return 0u;
}
std::uint32_t PlayerManagerAccessRuntimeStub::Allocate(std::uint32_t) {
    return 0u;
}
std::uint32_t PlayerManagerAccessRuntimeStub::Construct(std::uint32_t) {
    return 0u;
}

std::uint32_t PlayerManagerAccess::Get(
    PlayerManagerAccessState& state, IPlayerManagerAccessRuntime& runtime) {
    if ((state.guard & 1u) != 0u)
        return state.instance;
    state.guard = static_cast<std::uint8_t>(state.guard | 1u);
    const auto manager = runtime.SingletonManager();
    state.instance = runtime.Register(manager, "palPlayerManager",
        kSingletonId, kFactoryAddress, kRegistrationArgument);
    return state.instance;
}

std::uint32_t PlayerManagerAccess::CreateInstance(
    IPlayerManagerAccessRuntime& runtime) {
    const auto address = runtime.Allocate(kObjectBytes);
    return address != 0u ? runtime.Construct(address) : 0u;
}

}  // namespace pal4
