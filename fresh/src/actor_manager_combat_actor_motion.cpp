#include "pal4/actor_manager_combat_actor_motion.h"
#include "pal4/actor_manager_float_clamp.h"

#include <algorithm>

namespace pal4 {
namespace {

void ClampComponents(ActorManagerVec3& vector) {
    vector.x = std::clamp(vector.x, -1.0f, 1.0f);
    vector.y = std::clamp(vector.y, -1.0f, 1.0f);
    (void)ActorManagerFloatClamp::Clamp(vector.z, -1.0f, 1.0f);
}

float Dot(const ActorManagerVec3& first, const ActorManagerVec3& second) {
    return first.x * second.x + first.y * second.y + first.z * second.z;
}

class IteratorOwner final {
public:
    IteratorOwner(std::uint32_t value, IActorManagerCombatMotionRuntime& runtime)
        : value_(value), runtime_(runtime) {}
    ~IteratorOwner() {
        if (value_) runtime_.DeleteIterator(value_, 1u);
    }
    [[nodiscard]] std::uint32_t Get() const { return value_; }

private:
    std::uint32_t value_;
    IActorManagerCombatMotionRuntime& runtime_;
};

}  // namespace

void ActorManagerCombatActorMotion::Update(
    const std::uint32_t actor, const std::uint32_t flag,
    IActorManagerCombatMotionRuntime& runtime) {
    const auto child = runtime.ReadWord(actor + 808u);
    if (child && runtime.IsActorReady(actor)) {
        if (static_cast<std::uint8_t>(flag))
            runtime.ChildSlot4(child, runtime.ActorSlot444(actor, 0u));

        ActorManagerVec3 direction{-33.0f, 0.0f, 304.0f};
        runtime.Normalize(direction);
        const ActorManagerVec3 destination{
            8.0f + direction.x * 300.0f,
            direction.y * 300.0f,
            156.0f + direction.z * 300.0f};
        ActorManagerVec3 toward{
            destination.x - runtime.ChildPosition(actor).x,
            destination.y - runtime.ChildPosition(actor).y,
            destination.z - runtime.ChildPosition(actor).z};
        runtime.Normalize(toward);
        ClampComponents(toward);
        ActorManagerVec3 forward{0.0f, 0.0f, 1.0f};
        runtime.Normalize(forward);
        ClampComponents(forward);
        const double degrees = runtime.Acos(Dot(toward, forward)) *
                               57.29577951308232;
        const float signed_angle = static_cast<float>(
            toward.x < 0.0f ? -degrees : degrees);
        (void)runtime.ChildSlot8(
            actor, ActorManagerVec3{0.0f, signed_angle, 0.0f}, 0u);
        runtime.ActorSlot564(actor, kGlobalMotion, 1u);
    }

    if (!runtime.ReadByte(actor + 1160u)) return;
    runtime.WriteByte(actor + 1160u, 0u);
    const IteratorOwner iterator(runtime.ActorSlot464(actor), runtime);
    if (!iterator.Get()) return;
    while (!runtime.IteratorAtEnd(iterator.Get())) {
        const auto member = runtime.IteratorActor(iterator.Get());
        const auto member_child = runtime.ReadWord(member + 808u);
        if (member_child) {
            const auto name = runtime.GlobalAnimationName().value_or("");
            if (runtime.EqualIgnoreCase(
                    name, runtime.AnimationName(member_child + 368u))) {
                runtime.WriteByte(member + 1160u, 0u);
                runtime.ActorSlot396(member, 1u);
            }
        }
        runtime.IteratorNext(iterator.Get());
    }
}

std::uint32_t ActorManagerCombatActorMotion::StartAnimation(
    const std::uint32_t actor, IActorManagerCombatMotionRuntime& runtime) {
    runtime.ActorSlot396(actor, 1u);
    const auto result = runtime.ChildSlot8(
        actor, ActorManagerVec3{0.0f, 180.0f, 0.0f}, 1u);
    runtime.WriteByte(actor + 1160u, 1u);
    return result;
}

}  // namespace pal4
