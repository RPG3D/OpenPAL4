#include "pal4/actor_manager_player_scene_transition.h"

#include <cctype>

namespace pal4 {

namespace {

bool EqualIgnoringCase(std::string_view left, std::string_view right) {
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0u; i < left.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(left[i])) !=
            std::tolower(static_cast<unsigned char>(right[i])))
            return false;
    }
    return true;
}

class IteratorOwner final {
public:
    IteratorOwner(const std::uint32_t iterator,
                  IActorManagerPlayerSceneRuntime& runtime)
        : iterator_(iterator), runtime_(runtime) {}
    ~IteratorOwner() {
        if (iterator_) runtime_.DestroyIterator(iterator_);
    }
    IteratorOwner(const IteratorOwner&) = delete;
    IteratorOwner& operator=(const IteratorOwner&) = delete;

private:
    std::uint32_t iterator_;
    IActorManagerPlayerSceneRuntime& runtime_;
};

}  // namespace

void ActorManagerPlayerSceneTransition::Update(
    const std::uint32_t actor, const std::uint32_t flags,
    IActorManagerPlayerSceneRuntime& runtime) {
    if (runtime.ReadWord(actor + 808u) && runtime.IsActorReady(actor)) {
        if (static_cast<std::uint8_t>(flags)) {
            const auto transform = runtime.ResolveAnimationTransform(actor, 0u);
            runtime.SetAnimationTransform(runtime.ReadWord(actor + 808u),
                                          transform);
        }

        const float yaw = runtime.ActorSlot(actor) == 2u ? 206.0f : 180.0f;
        static_cast<void>(runtime.MoveChild(actor, {0.0f, yaw, 0.0f}, 0u));
        static_cast<void>(runtime.QueryCurrentStat(actor, 7u));
        static_cast<void>(runtime.SecondaryState(actor));
        if (runtime.PrimaryState(actor) == 4u ||
            runtime.PrimaryState(actor) == 1u) {
            runtime.SelectAnimation(actor, kPrimaryAnimation, 1u);
        } else if (runtime.SecondaryState(actor) == 2u) {
            runtime.SelectAnimation(actor, kSecondaryAnimation, 1u);
        } else {
            runtime.SelectAnimation(actor, kDefaultAnimation, 1u);
        }
    }

    if (!runtime.ReadByte(actor + 1308u)) return;
    runtime.WriteByte(actor + 1308u, 0u);
    const auto iterator = runtime.CreateActorIterator(actor);
    if (!iterator) return;
    IteratorOwner owner(iterator, runtime);
    while (!runtime.IteratorDone(iterator)) {
        const auto other = runtime.IteratorActor(iterator);
        const auto animation = runtime.ReadWord(other + 808u);
        if (animation && EqualIgnoringCase(runtime.MatchAnimationName(),
                                           runtime.AnimationName(animation))) {
            runtime.WriteByte(other + 1308u, 0u);
            runtime.ActivateActor(other, 1u);
        }
        runtime.AdvanceIterator(iterator);
    }
}

std::uint32_t ActorManagerPlayerSceneTransition::Begin(
    const std::uint32_t actor,
    IActorManagerPlayerSceneRuntime& runtime) {
    runtime.ActivateActor(actor, 1u);
    const auto result = runtime.MoveChild(actor, {0.0f, 180.0f, 0.0f}, 1u);
    runtime.WriteByte(actor + 1308u, 1u);
    return result;
}

}  // namespace pal4
