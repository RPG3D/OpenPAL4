#include "pal4/monster_action_dispatcher.h"

namespace pal4 {

std::uint32_t MonsterActionDispatchRuntimeStub::ManagerInstance() {
    return 0u;
}

std::uint32_t MonsterActionDispatchRuntimeStub::IterateAndCallback(
    std::uint32_t, std::uint32_t, std::uint32_t) {
    return 0u;
}

std::uint32_t MonsterActionDispatcher::Process(
    std::uint32_t manager_address,
    IMonsterActionDispatchRuntime& runtime) {
    return runtime.IterateAndCallback(
        runtime.ManagerInstance(), kObjectInteractionCallback,
        manager_address);
}

}  // namespace pal4
