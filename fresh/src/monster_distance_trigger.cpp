#include "pal4/monster_distance_trigger.h"

#include "pal4/monster_behavior_manager.h"

#include <bit>
#include <cmath>
#include <cctype>

namespace pal4 {

double MonsterDistanceTriggerRuntimeStub::Distance(
    const std::array<float, 3>& delta) {
    const float squared = delta[0] * delta[0] +
                          delta[1] * delta[1] +
                          delta[2] * delta[2];
    return std::sqrt(static_cast<double>(squared));
}
void MonsterDistanceTriggerRuntimeStub::NotifyAt564(std::uint32_t) {}
std::string MonsterDistanceTriggerRuntimeStub::AnimationName(
    std::uint32_t) { return {}; }
bool MonsterDistanceTriggerRuntimeStub::NameEqualsIgnoreCase(
    std::string_view left, std::string_view right) {
    if (left.size() != right.size())
        return false;
    for (std::size_t index = 0; index < left.size(); ++index) {
        if (std::tolower(static_cast<unsigned char>(left[index])) !=
            std::tolower(static_cast<unsigned char>(right[index])))
            return false;
    }
    return true;
}
bool MonsterDistanceTriggerRuntimeStub::SelectAnimation(
    std::uint32_t, std::string_view) { return false; }

std::uint32_t MonsterDistanceTrigger::Process(
    MonsterDistanceTriggerState& trigger, std::uint32_t trigger_address,
    MonsterBehaviorManagerState& manager, std::uint32_t manager_address,
    IMonsterDistanceTriggerRuntime& runtime) {
    if (trigger.active == 0u)
        return trigger_address;

    const std::array<float, 3> delta{
        trigger.position[0] -
            std::bit_cast<float>(manager.words[0xacu / 4u]),
        trigger.position[1] -
            std::bit_cast<float>(manager.words[0xb0u / 4u]),
        trigger.position[2] -
            std::bit_cast<float>(manager.words[0xb4u / 4u])};
    const auto distance = runtime.Distance(delta);
    if (trigger.kind == 2u) {
        if (distance <= 150.0) {
            manager.words[0x730u / 4u] = 2u;
            manager.words[0x734u / 4u] = std::bit_cast<std::uint32_t>(10.0f);
            manager.words[0x738u / 4u] = 0u;
        }
        return trigger_address;
    }
    if (trigger.kind != 0u) {
        manager.words[0x730u / 4u] = 0u;
        manager.words[0x734u / 4u] = 0u;
        manager.words[0x738u / 4u] = 0u;
        return trigger_address;
    }
    if (!(distance <= 40.0))
        return trigger_address;

    trigger.active = 0u;
    manager.words[0x730u / 4u] = 1u;
    manager.words[0x734u / 4u] = std::bit_cast<std::uint32_t>(5.0f);
    manager.words[0x738u / 4u] = 0u;
    runtime.NotifyAt564(manager_address + 0x564u);
    const auto animation = manager_address + 0x170u;
    if (runtime.NameEqualsIgnoreCase(runtime.AnimationName(animation),
                                     "C01"))
        return trigger_address;
    if (runtime.NameEqualsIgnoreCase(runtime.AnimationName(animation),
                                     "Z01"))
        return trigger_address;
    if (!runtime.SelectAnimation(animation, "C01"))
        (void)runtime.SelectAnimation(animation, "Z01");
    return trigger_address;
}

}  // namespace pal4
